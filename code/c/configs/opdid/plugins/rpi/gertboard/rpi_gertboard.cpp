
// OPDID plugin that supports the Gertboard on Raspberry Pi

#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include "Poco/Tuple.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "gb_common.h"
#include "gb_spi.h"
#include "gb_pwm.h"

#include "../rpi.h"

#include "LinuxOPDID.h"

// mapping to different revisions of RPi
// first number indicates Gertboard label (GPxx), second specifies internal pin number
static int pinMapRev1[][2] = { 
	{0, 0}, {1, 1}, {4, 4}, {7, 7}, {8, 8}, {9, 9}, 
	{10, 10}, {11, 11}, {14, 14}, {15, 15}, {17, 17}, 
	{18, 18}, {21, 21}, {22, 22}, {23, 23}, {24, 24}, {25, 25},
	{-1, -1}
};

static int pinMapRev2[][2] = { 
	{0, 2}, {1, 3}, {4, 4}, {7, 7}, {8, 8}, {9, 9}, 
	{10, 10}, {11, 11}, {14, 14}, {15, 15}, {17, 17}, 
	{18, 18}, {21, 27}, {22, 22}, {23, 23}, {24, 24}, {25, 25},
	{-1, -1}
};

///////////////////////////////////////////////////////////////////////////////
// GertboardPlugin: Plugin for providing Gertboard resources to OPDID
///////////////////////////////////////////////////////////////////////////////

class GertboardPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;
	std::string nodeID;

	int (*pinMap)[][2];		// the map to use for mapping Gertboard pins to internal pins

	std::string serialDevice;
	uint32_t serialTimeoutMs;
	// At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream;
	bool expanderInitialized;

	// translates external pin IDs to an internal pin; throws an exception if the pin cannot
	// be mapped or the resource is already used
	int mapAndLockPin(int pinNumber, std::string forNode);

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

	virtual void sendExpansionPortCode(uint8_t code);
	virtual uint8_t receiveExpansionPortCode(void);
};
///////////////////////////////////////////////////////////////////////////////
// DigitalGertboardPort: Represents a digital input/output pin on the Gertboard.
///////////////////////////////////////////////////////////////////////////////

class DigitalGertboardPort : public OPDI_DigitalPort {
protected:
	AbstractOPDID *opdid;
	int pin;
public:
	DigitalGertboardPort(AbstractOPDID *opdid, const char *ID, int pin);
	virtual ~DigitalGertboardPort(void);
	virtual void setLine(uint8_t line) override;
	virtual void setMode(uint8_t mode) override;
	virtual void getState(uint8_t *mode, uint8_t *line) override;
};

DigitalGertboardPort::DigitalGertboardPort(AbstractOPDID *opdid, const char *ID, int pin) : OPDI_DigitalPort(ID, 
	(std::string("Digital Gertboard Port ") + to_string(pin)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_BIDI,	// default: input
	0) {
	this->opdid = opdid;
	this->pin = pin;
}

DigitalGertboardPort::~DigitalGertboardPort(void) {
	// release resources; configure as floating input
	// configure as floating input
	INP_GPIO(this->pin);

	GPIO_PULL = 0;
	short_wait();
	GPIO_PULLCLK0 = (1 < this->pin);
	short_wait();
	GPIO_PULL = 0;
	GPIO_PULLCLK0 = 0;
}

void DigitalGertboardPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	if (line == 0) {
		GPIO_CLR0 = (1 << this->pin);
	} else {
		GPIO_SET0 = (1 << this->pin);
	}
}

void DigitalGertboardPort::setMode(uint8_t mode) {

	// cannot set pulldown mode
	if (mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN)
		throw PortError("Digital Gertboard Port does not support pulldown mode");

	OPDI_DigitalPort::setMode(mode);

	if (this->mode == OPDI_DIGITAL_MODE_INPUT_FLOATING) {
		// configure as floating input
		INP_GPIO(this->pin);

		GPIO_PULL = 0;
		short_wait();
		GPIO_PULLCLK0 = (1 < this->pin);
		short_wait();
		GPIO_PULL = 0;
		GPIO_PULLCLK0 = 0;
	} else
	if (this->mode == OPDI_DIGITAL_MODE_INPUT_PULLUP) {
		// configure as input with pullup
		INP_GPIO(this->pin);

		GPIO_PULL = 2;
		short_wait();
		GPIO_PULLCLK0 = (1 < this->pin);
		short_wait();
		GPIO_PULL = 0;
		GPIO_PULLCLK0 = 0;
	} else {
		// configure as output
		INP_GPIO(this->pin); 
		OUT_GPIO(this->pin);
	}
}

void DigitalGertboardPort::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;

	// read line
	unsigned int b = GPIO_IN0;
	// bit set?
	*line = (b & (1 << this->pin)) == (unsigned int)(1 << this->pin) ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////
// AnalogGertboardOutput: Represents an analog output pin on the Gertboard.
///////////////////////////////////////////////////////////////////////////////

class AnalogGertboardOutput : public OPDI_AnalogPort {
protected:
	AbstractOPDID *opdid;
	int output;
public:
	AnalogGertboardOutput(AbstractOPDID *opdid, const char *id, int output);

	virtual void setFlags(int32_t flags) override;
	virtual void setMode(uint8_t mode) override;
	virtual void setResolution(uint8_t resolution) override;
	virtual void setReference(uint8_t reference) override;
	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value) override;
	virtual void getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) override;
};

AnalogGertboardOutput::AnalogGertboardOutput(AbstractOPDID *opdid, const char *id, int output) : OPDI_AnalogPort(id, 
	(std::string("Analog Gertboard Output ") + to_string(output)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_OUTPUT, 
	// possible resolutions - hardware decides which one is actually used; set value in configuration
	OPDI_ANALOG_PORT_RESOLUTION_8 | OPDI_ANALOG_PORT_RESOLUTION_10 | OPDI_ANALOG_PORT_RESOLUTION_12) {

	this->opdid = opdid;
	this->mode = 1;
	this->resolution = 8;	// most Gertboards apparently use an 8 bit DAC; but this can be changed in the configuration
	this->reference = 0;
	this->value = 0;

	// check valid output
	if ((output < 0) || (output > 1))
		throw Poco::DataException("Invalid analog output channel number (expected 0 or 1): " + to_string(output));
	this->output = output;

	// setup analog output port
	INP_GPIO(7);  SET_GPIO_ALT(7,0);
	INP_GPIO(9);  SET_GPIO_ALT(9,0);
	INP_GPIO(10); SET_GPIO_ALT(10,0);
	INP_GPIO(11); SET_GPIO_ALT(11,0);

	// Setup SPI bus
	setup_spi();

	write_dac(this->output, this->value);
}

void AnalogGertboardOutput::setFlags(int32_t flags) {
	// ignore flag changes
}

// function that handles the set direction command (opdi_set_digital_port_mode)
void AnalogGertboardOutput::setMode(uint8_t mode) {
	throw PortError("Gertboard analog output mode cannot be changed");
}

void AnalogGertboardOutput::setResolution(uint8_t resolution) {
	OPDI_AnalogPort::setResolution(resolution);
}

void AnalogGertboardOutput::setReference(uint8_t reference) {
	throw PortError("Gertboard analog output reference cannot be changed");
}

void AnalogGertboardOutput::setValue(int32_t value) {
	OPDI_AnalogPort::setValue(value);

	write_dac(this->output, this->value);
}

// function that fills in the current port state
void AnalogGertboardOutput::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) {
	*mode = this->mode;
	*resolution = this->resolution;
	*reference = this->reference;
	// set remembered value
	*value = this->value;
}

///////////////////////////////////////////////////////////////////////////////
// AnalogGertboardInput: Represents an analog input pin on the Gertboard.
///////////////////////////////////////////////////////////////////////////////

class AnalogGertboardInput : public OPDI_AnalogPort {
protected:
	AbstractOPDID *opdid;
	int input;
public:
	AnalogGertboardInput(AbstractOPDID *opdid, const char *id, int input);

	virtual void setFlags(int32_t flags) override;
	virtual void setMode(uint8_t mode) override;
	virtual void setResolution(uint8_t resolution) override;
	virtual void setReference(uint8_t reference) override;
	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value) override;
	virtual void getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) override;
};

AnalogGertboardInput::AnalogGertboardInput(AbstractOPDID *opdid, const char *id, int input) : OPDI_AnalogPort(id, 
	(std::string("Analog Gertboard Input ") + to_string(input)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_INPUT, 
	// possible resolutions - hardware decides which one is actually used; set value in configuration
	OPDI_ANALOG_PORT_RESOLUTION_8 | OPDI_ANALOG_PORT_RESOLUTION_10 | OPDI_ANALOG_PORT_RESOLUTION_12) {

	this->opdid = opdid;
	this->mode = 0;
	this->resolution = 8;	// most Gertboards apparently use an 8 bit DAC; but this can be changed in the configuration
	this->reference = 0;
	this->value = 0;

	// check valid input
	if ((input < 0) || (input > 1))
		throw Poco::DataException("Invalid analog input channel number (expected 0 or 1): " + to_string(input));
	this->input = input;

	// setup analog input port
	INP_GPIO(8);  SET_GPIO_ALT(8,0);
	INP_GPIO(9);  SET_GPIO_ALT(9,0);
	INP_GPIO(10); SET_GPIO_ALT(10,0);
	INP_GPIO(11); SET_GPIO_ALT(11,0);

	// Setup SPI bus
	setup_spi();
}

void AnalogGertboardInput::setFlags(int32_t flags) {
	// ignore flag changes
}

// function that handles the set direction command (opdi_set_digital_port_mode)
void AnalogGertboardInput::setMode(uint8_t mode) {
	throw PortError("Gertboard analog input mode cannot be changed");
}

void AnalogGertboardInput::setResolution(uint8_t resolution) {
	OPDI_AnalogPort::setResolution(resolution);
}

void AnalogGertboardInput::setReference(uint8_t reference) {
	throw PortError("Gertboard analog input reference cannot be changed");
}

void AnalogGertboardInput::setValue(int32_t value) {
	throw PortError("Gertboard analog input value cannot be set");
}

// function that fills in the current port state
void AnalogGertboardInput::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) {
	*mode = this->mode;
	*resolution = this->resolution;
	*reference = this->reference;
	// read value from ADC; correct range
	this->value = OPDI_AnalogPort::validateValue(read_adc(this->input));
	// set remembered value
	*value = this->value;
}

///////////////////////////////////////////////////////////////////////////////
// GertboardButton: Represents a button on the Gertboard.
// If the button is pressed, a connected master will be notified to update
// its state. This port permanently queries the state of the button's pin.
///////////////////////////////////////////////////////////////////////////////

class GertboardButton : public OPDI_DigitalPort {
protected:
	AbstractOPDID *opdid;
	int pin;
	uint8_t lastQueriedState;
	uint64_t lastQueryTime;
	uint64_t queryInterval;
	uint64_t lastRefreshTime;
	uint64_t refreshInterval;

	virtual uint8_t doWork(uint8_t canSend) override;
	virtual uint8_t queryState(void);
public:
	GertboardButton(AbstractOPDID *opdid, const char *ID, int pin);
	virtual void setLine(uint8_t line) override;
	virtual void setMode(uint8_t mode) override;
	virtual void setDirCaps(const char *dirCaps) override;
	virtual void setFlags(int32_t flags) override;
	virtual void getState(uint8_t *mode, uint8_t *line) override;
};

GertboardButton::GertboardButton(AbstractOPDID *opdid, const char *ID, int pin) : OPDI_DigitalPort(ID, 
	(std::string("Gertboard Button on pin ") + to_string(pin)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_INPUT,	// default: input with pullup always on
	OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) {
	this->opdid = opdid;
	this->pin = pin;
	this->mode = OPDI_DIGITAL_MODE_INPUT_PULLUP;

	// configure as input with pullup
	INP_GPIO(this->pin);
	GPIO_PULL = 2;
	short_wait();
	GPIO_PULLCLK0 = (1 << this->pin);
	short_wait();
	GPIO_PULL = 0;
	GPIO_PULLCLK0 = 0;

	this->lastQueryTime = opdi_get_time_ms();
	this->lastQueriedState = this->queryState();
	this->queryInterval = 10;	// milliseconds
	// set the refresh interval to a reasonably high number
	// to avoid dos'ing the master with refresh requests in case
	// the button pin toggles too fast
	this->refreshInterval = 10;
}

// main work function of the button port - regularly called by the OPDID system
uint8_t GertboardButton::doWork(uint8_t canSend) {
	OPDI_DigitalPort::doWork(canSend);

	// query interval not yet reached?
	if (opdi_get_time_ms() - this->lastQueryTime < this->queryInterval)
		return OPDI_STATUS_OK;

	// query now
	this->lastQueryTime = opdi_get_time_ms();
	uint8_t line = this->queryState();
	// current state different from last submitted state?
	if (this->lastQueriedState != line) {
		this->lastQueriedState = line;
		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log(std::string("Gertboard Button change detected: ") + this->id);
		// refresh interval not exceeded?
		if (opdi->isConnected() && (opdi_get_time_ms() - this->lastRefreshTime > this->refreshInterval)) {
			this->lastRefreshTime = opdi_get_time_ms();
			// notify master to refresh this port's state
			this->refreshRequired = true;
			this->lastRefreshTime = opdi_get_time_ms();
		}
	}

	return OPDI_STATUS_OK;
}

uint8_t GertboardButton::queryState(void) {
//	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
//		opdid->log("Querying Gertboard button pin " + to_string(this->pin) + " (" + this->id + ")");

	// read line
	unsigned int b = GPIO_IN0;
	// bit set?
	// button logic is inverse (low = pressed)
	uint8_t result = (b & (1 << this->pin)) == (unsigned int)(1 << this->pin) ? 0 : 1;

//	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
//		opdid->log("Gertboard button pin state is " + to_string((int)result));

	return result;
}

void GertboardButton::setLine(uint8_t line) {
	opdid->log("Warning: Gertboard Button has no output to be changed, ignoring");
}

void GertboardButton::setMode(uint8_t mode) {
	opdid->log("Warning: Gertboard Button mode cannot be changed, ignoring");
}

void GertboardButton::setDirCaps(const char *dirCaps) {
	opdid->log("Warning: Gertboard Button direction capabilities cannot be changed, ignoring");
}

void GertboardButton::setFlags(int32_t flags) {
	// TODO validate?
	OPDI_DigitalPort::setFlags(flags);
}

void GertboardButton::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;
	// remember queried line state
	*line = this->lastQueriedState;
}

///////////////////////////////////////////////////////////////////////////////
// GertboardPWM: Represents a PWM pin on the Gertboard.
// Pin 18 is currently the only pin that supports hardware PWM.
///////////////////////////////////////////////////////////////////////////////

class GertboardPWM: public OPDI_DialPort {
protected:
	AbstractOPDID *opdid;
	int pin;
	bool inverse;
public:
	GertboardPWM(AbstractOPDID *opdid, const int pin, const char *ID, bool inverse);
	virtual ~GertboardPWM(void);
	virtual void setPosition(int64_t position) override;
};

GertboardPWM::GertboardPWM(AbstractOPDID *opdid, const int pin, const char *ID, bool inverse) : OPDI_DialPort(ID) {
	if (pin != 18)
		throw Poco::ApplicationException("GertboardPWM only supports pin 18");
	this->opdid = opdid;
	this->pin = pin;
	this->inverse = inverse;
	this->minValue = 0;
	// use 10 bit PWM
	this->maxValue = 1024;
	this->step = 1;

	// initialize PWM
	INP_GPIO(this->pin);  SET_GPIO_ALT(this->pin, 5);
	setup_pwm();
	force_pwm0(0, PWM0_ENABLE);
}

GertboardPWM::~GertboardPWM(void) {
	// stop PWM when the port is freed
//	this->opdid->log("Freeing GertboardPWM port; stopping PWM");

	pwm_off();
}

void GertboardPWM::setPosition(int64_t position) {
	// calculate nearest position according to step
	OPDI_DialPort::setPosition(position);

	// set PWM value; inverse polarity if specified
	force_pwm0(this->position, PWM0_ENABLE | (this->inverse ? PWM0_REVPOLAR : 0));
}


///////////////////////////////////////////////////////////////////////////////
// DigitalExpansionPort: A digital input/output pin on the Atmega328P on the Gertboard.
// Requires that the AtmegaPortExpander software runs on the microcontroller.
// Communicates via RS232, so the connections between GP14/15 and MCRX/TX (pins PD0/PD1)
// must be present. The virtual port definitions are:

#define VP0(reg) BIT(D,2,reg)
#define VP1(reg) BIT(D,3,reg)
#define VP2(reg) BIT(D,4,reg)
#define VP3(reg) BIT(D,5,reg)
#define VP4(reg) BIT(D,6,reg)
#define VP5(reg) BIT(D,7,reg)
#define VP6(reg) BIT(B,0,reg)
#define VP7(reg) BIT(B,1,reg)
#define VP8(reg) BIT(B,2,reg)
#define VP9(reg) BIT(B,3,reg)
#define VP10(reg) BIT(B,4,reg)
#define VP11(reg) BIT(B,5,reg)
#define VP12(reg) BIT(C,0,reg)
#define VP13(reg) BIT(C,1,reg)
#define VP14(reg) BIT(C,2,reg)
#define VP15(reg) BIT(C,3,reg)

///////////////////////////////////////////////////////////////////////////////

#define OUTPUT	6
#define PULLUP	5
#define LINESTATE 4
#define PORTMASK 0x0f

#define SIGNALCODE	0xff
#define MAGIC		"OPDIDGBPEINIT"
#define DEACTIVATE	128

class DigitalExpansionPort : public OPDI_DigitalPort {
friend class GertboardPlugin;
protected:
	// Specifies the pin behaviour in output mode.
	enum DriverType {
		/* Standard behaviour: if Low, the pin is connected to internal ground (current sink).
		If High, the pin is connected to internal Vcc (current source). */
		STANDARD,
		/* Low side driver: if Low, the pin is floating.
		If High, the pin is connected to internal ground (current sink). */
		LOW_SIDE,
		/* High side driver: if Low, the pin is floating.
		If High, the pin is connected to internal Vcc (current source). */
		HIGH_SIDE
	};

	AbstractOPDID *opdid;
	GertboardPlugin *gbPlugin;
	int pin;
	DriverType driverType;

public:
	DigitalExpansionPort(AbstractOPDID *opdid, GertboardPlugin *gbPlugin, const char *ID, int pin);
	virtual ~DigitalExpansionPort(void);
	virtual void setLine(uint8_t line) override;
	virtual void setMode(uint8_t mode) override;
	virtual void getState(uint8_t *mode, uint8_t *line) override;
};

DigitalExpansionPort::DigitalExpansionPort(AbstractOPDID *opdid, GertboardPlugin *gbPlugin, const char *ID, int pin) : OPDI_DigitalPort(ID, 
	(std::string("Digital Expansion Port ") + to_string(pin)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_BIDI,	// default: bidirectional
	0) {
	this->opdid = opdid;
	this->gbPlugin = gbPlugin;
	this->pin = pin;
	this->driverType = STANDARD;
}

DigitalExpansionPort::~DigitalExpansionPort(void) {
}

void DigitalExpansionPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	uint8_t code = this->pin;

	if (this->driverType == STANDARD) {
		// set output flag
		code |= (1 << OUTPUT);
		// set High or Low accordingly
		if (this->line == 1) {
			code |= (1 << LINESTATE);
		}
	} else
	if (this->driverType == LOW_SIDE) {
		// active (High)?
		if (this->line == 1) {
			// set to a Low output
			code |= (1 << OUTPUT);
		} else {
			// set to a floating input (no action required)
		}
	} else
	if (this->driverType == HIGH_SIDE) {
		// active (High)?
		if (this->line == 1) {
			// set to a Low output
			code |= (1 << OUTPUT);
			code |= (1 << LINESTATE);
		} else {
			// set to a floating input (no action required)
		}
	} else
		throw PortError("Unknown driver type: " + to_string(driverType));

	this->gbPlugin->sendExpansionPortCode(code);
	uint8_t returnCode = this->gbPlugin->receiveExpansionPortCode();
	if ((returnCode & PORTMASK) != (code & PORTMASK))
		throw PortError("Expansion port communication failure");
}

void DigitalExpansionPort::setMode(uint8_t mode) {
	// cannot set pulldown mode
	if (mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN)
		throw PortError("Digital Expansion Port does not support pulldown mode");

	OPDI_DigitalPort::setMode(mode);

	uint8_t code = this->pin;

	if (this->mode == OPDI_DIGITAL_MODE_INPUT_FLOATING) {
		// configure as floating input
	} else
	if (this->mode == OPDI_DIGITAL_MODE_INPUT_PULLUP) {
		code |= (1 << PULLUP);
	} else {
		if (this->driverType == STANDARD) {
			// set output flag
			code |= (1 << OUTPUT);
			// set High or Low accordingly
			if (this->line == 1) {
				code |= (1 << LINESTATE);
			}
		} else
		if (this->driverType == LOW_SIDE) {
			// active (High)?
			if (this->line == 1) {
				// set to a Low output
				code |= (1 << OUTPUT);
			} else {
				// set to a floating input (no action required)
			}
		} else
		if (this->driverType == HIGH_SIDE) {
			// active (High)?
			if (this->line == 1) {
				// set to a Low output
				code |= (1 << OUTPUT);
				code |= (1 << LINESTATE);
			} else {
				// set to a floating input (no action required)
			}
		} else
			throw PortError("Unknown driver type: " + to_string(driverType));
	}

	this->gbPlugin->sendExpansionPortCode(code);
	uint8_t returnCode = this->gbPlugin->receiveExpansionPortCode();
	if ((returnCode & PORTMASK) != (code & PORTMASK))
		throw PortError("Expansion port communication failure");
}

void DigitalExpansionPort::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;

	if (this->mode == OPDI_DIGITAL_MODE_OUTPUT) {
		// return remembered state
		*line = this->line;
	} else {
		// read line state from the port expander
		uint8_t code = this->pin;

		if (this->mode == OPDI_DIGITAL_MODE_INPUT_PULLUP) {
			code |= (1 << PULLUP);
		}

		this->gbPlugin->sendExpansionPortCode(code);
		uint8_t returnCode = this->gbPlugin->receiveExpansionPortCode();

		if ((returnCode & ~(1 << LINESTATE)) != code)
			throw PortError("Expansion port communication failure");

		if ((returnCode & (1 << LINESTATE)) == (1 << LINESTATE))
			*line = 1;
		else
			*line = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// GertboardPlugin: Plugin for providing Gertboard resources to OPDID
///////////////////////////////////////////////////////////////////////////////

int GertboardPlugin::mapAndLockPin(int pinNumber, std::string forNode) {
	int i = 0;
	int internalPin = -1;
	// linear search through pin definitions; map to internal pin if found
	while ((*pinMap)[i][0] >= 0) {
		if ((*pinMap)[i][0] == pinNumber) {
			internalPin = (*pinMap)[i][1];
			break;
		}
		i++;
	}
	if (internalPin < 0)
		throw Poco::DataException(std::string("The pin is not supported: ") + this->opdid->to_string(pinNumber));

	// try to lock the pin as a resource
	this->opdid->lockResource(RPI_GPIO_PREFIX + this->opdid->to_string(internalPin), forNode);

	return internalPin;
}

void GertboardPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;
	this->nodeID = node;
	this->expanderInitialized = false;

	Poco::Util::AbstractConfiguration *nodeConfig = config->createView(node);

	// try to lock the whole Gertboard as a resource
	// to avoid trouble repeatedly initializing IO
	this->opdid->lockResource(std::string("Gertboard"), node);

	// prepare Gertboard IO (requires root permissions)
	setup_io();

	// determine pin map to use
	this->pinMap = (int (*)[][2])&pinMapRev1;
	std::string revision = abstractOPDID->getConfigString(nodeConfig, "Revision", "1", false);
	if (revision == "1") {
		// default
	} else
	if (revision == "2") {
		this->pinMap = (int (*)[][2])&pinMapRev2;
	} else
		throw Poco::DataException("Gertboard revision not supported (no pin map; use 1 or 2); Revision = " + revision);	

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");

	// to use the expansion ports we need to have a serial device name
	// if this is not configured we can't use the serial port expansion
	this->serialDevice = nodeConfig->getString("SerialDevice", "");

	// if serial device specified, open and configure it
	if (this->serialDevice != "") {

		// try to lock the serial device as a resource
		this->opdid->lockResource(this->serialDevice, node);

		int timeout = nodeConfig->getInt("SerialTimeout", 100);
		if (timeout < 0)
			throw Poco::DataException("SerialTimeout may not be negative: " + this->opdid->to_string(timeout));

		this->serialTimeoutMs = timeout;

		this->uart0_filestream = open(this->serialDevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);		// open in non blocking read/write mode
		if (this->uart0_filestream == -1)
			throw Poco::Exception("Unable to open serial device " + this->serialDevice);

		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B19200 | CS8 | CLOCAL | CREAD;		// set baud rate
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);

		// the serial device, if present, uses pins 14 and 15, so these need to be locked
		// if other ports try to use these ports it will fail
		this->mapAndLockPin(14, node + " SerialDevice Port Expansion");
		this->mapAndLockPin(15, node + " SerialDevice Port Expansion");

		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log(node + ": SerialDevice " + this->serialDevice + " setup successfully with a timeout of " + this->opdid->to_string(this->serialTimeoutMs) + " ms");
	}

	// the Gertboard plugin node expects a list of node names that determine the ports that this plugin provides

	// enumerate keys of the plugin's nodes (in specified order)
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("Enumerating Gertboard nodes: " + node + ".Nodes");

	Poco::Util::AbstractConfiguration *nodes = config->createView(node + ".Nodes");

	// get ordered list of ports
	Poco::Util::AbstractConfiguration::Keys portKeys;
	nodes->keys("", portKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of port keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = portKeys.begin(); it != portKeys.end(); ++it) {
		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			nli++;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		this->opdid->log("Warning: No ports configured in node " + node + ".Nodes; is this intended?");
	}
	
	// go through items, create ports in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {
	
		std::string nodeName = nli->get<1>();
	
		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log("Setting up Gertboard port(s) for node: " + nodeName);
			
		// get port section from the configuration
		Poco::Util::AbstractConfiguration *portConfig = config->createView(nodeName);
	
		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, "Type", "", true);

		if (portType == "DigitalPort") {
			// read pin number
			int pinNumber = portConfig->getInt("Pin", -1);
			// check whether the pin is valid; determine internal pin
			if (pinNumber < 0)
				throw Poco::DataException("A 'Pin' greater or equal than 0 must be specified for a Gertboard Digital port");

			int internalPin = this->mapAndLockPin(pinNumber, nodeName);
			
			// setup the port instance and add it; use internal pin number
			DigitalGertboardPort *port = new DigitalGertboardPort(abstractOPDID, nodeName.c_str(), internalPin);
			// set default group: Gertboard's node's group
			port->setGroup(group);
			abstractOPDID->configureDigitalPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
		if (portType == "Button") {
			// read pin number
			int pinNumber = portConfig->getInt("Pin", -1);
			// check whether the pin is valid; determine internal pin
			if (pinNumber < 0)
				throw Poco::DataException("A 'Pin' greater or equal than 0 must be specified for a Gertboard Button port");

			int internalPin = this->mapAndLockPin(pinNumber, nodeName);

			// setup the port instance and add it; use internal pin number
			GertboardButton *port = new GertboardButton(abstractOPDID, nodeName.c_str(), internalPin);
			// set default group: Gertboard's node's group
			port->setGroup(group);
			abstractOPDID->configureDigitalPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
		if (portType == "AnalogOutput") {
			// read output number
			int outputNumber = portConfig->getInt("Output", -1);
			// check whether the pin is valid; determine internal pin
			if ((outputNumber < 0) || (outputNumber > 1))
				throw Poco::DataException("An 'Output' of 0 or 1 must be specified for a Gertboard AnalogOutput port: " + abstractOPDID->to_string(outputNumber));

			// the analog output uses SPI; lock internal pins for SPI ports but ignore it if they are already locked
			// because another AnalogOutput or AnalogInput may also use SPI
			try {
				abstractOPDID->lockResource("7", nodeName);
				abstractOPDID->lockResource("9", nodeName);
				abstractOPDID->lockResource("10", nodeName);
				abstractOPDID->lockResource("11", nodeName);
			} catch (...) {}

			// lock analog output resource; this one may not be shared
			abstractOPDID->lockResource(std::string("AnalogOut") + abstractOPDID->to_string(outputNumber), nodeName);

			// setup the port instance and add it; use internal pin number
			AnalogGertboardOutput *port = new AnalogGertboardOutput(abstractOPDID, nodeName.c_str(), outputNumber);
			// set default group: Gertboard's node's group
			port->setGroup(group);
			abstractOPDID->configureAnalogPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
		if (portType == "AnalogInput") {
			// read input number
			int inputNumber = portConfig->getInt("Input", -1);
			// check whether the pin is valid; determine internal pin
			if ((inputNumber < 0) || (inputNumber > 1))
				throw Poco::DataException("An 'Input' of 0 or 1 must be specified for a Gertboard AnalogInput port: " + abstractOPDID->to_string(inputNumber));

			// the analog input uses SPI; lock internal pins for SPI ports but ignore it if they are already locked
			// because another AnalogOutput or AnalogInput may also use SPI
			try {
				abstractOPDID->lockResource("8", nodeName);
				abstractOPDID->lockResource("9", nodeName);
				abstractOPDID->lockResource("10", nodeName);
				abstractOPDID->lockResource("11", nodeName);
			} catch (...) {}

			// lock analog input resource; this one may not be shared
			abstractOPDID->lockResource(std::string("AnalogIn") + abstractOPDID->to_string(inputNumber), nodeName);

			// setup the port instance and add it; use internal pin number
			AnalogGertboardInput *port = new AnalogGertboardInput(abstractOPDID, nodeName.c_str(), inputNumber);
			// set default group: Gertboard's node's group
			port->setGroup(group);
			abstractOPDID->configureAnalogPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
		if (portType == "PWM") {
			// read inverse flag
			bool inverse = portConfig->getBool("Inverse", false);

			// the PWM output uses pin 18
			int internalPin = this->mapAndLockPin(18, nodeName);

			// setup the port instance and add it; use internal pin number
			GertboardPWM *port = new GertboardPWM(abstractOPDID, internalPin, nodeName.c_str(), inverse);
			// set default group: Gertboard's node's group
			port->setGroup(group);
			abstractOPDID->configurePort(portConfig, port, 0);
			
			int value = portConfig->getInt("Value", -1);
			if (value > -1) {
				port->setPosition(value);
			}
			abstractOPDID->addPort(port);
		} else
		if (portType == "DigitalExpansionPort") {
			// serial device must be configured first
			if (this->serialDevice == "")
				throw Poco::DataException("To use the port expansion, please set the SerialDevice parameter in the Gertboard node");
				
			// the expansion port must be activated by sending it a "magic" string
			// do this only once
			if (!this->expanderInitialized) {
				if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
					this->opdid->log(node + ": Initializing Atmega Port Expander");
					
				// check whether it's already initialized
				// for example if OPDID has crashed and is being restarted
				this->sendExpansionPortCode(SIGNALCODE);
				try {
					if (this->receiveExpansionPortCode() == SIGNALCODE)
						if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
							this->opdid->log(node + ": Port Expander is already initialized");
					this->expanderInitialized = true;		
				} catch (...) {}

				if (!this->expanderInitialized) {
					for (size_t i = 0; i < sizeof(MAGIC); i++) {
						this->sendExpansionPortCode(MAGIC[i]);
					}
					// try to receive the confirmation code
					if (this->receiveExpansionPortCode() != SIGNALCODE) {
						throw Poco::DataException("Port Expander did not respond to initialization");
					}
					if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
						this->opdid->log(node + ": Port Expander initialization sequence successfully completed");
					this->expanderInitialized = true;
				}
			}
		
			// read pin number
			int pinNumber = portConfig->getInt("Pin", -1);
			// check whether the pin is valid; determine internal pin
			if ((pinNumber < 0) || (pinNumber > 15))
				throw Poco::DataException("A 'Pin' number between 0 and 15 must be specified for a Gertboard Digital Expansion port");

			// try to lock the pin as a resource
			this->opdid->lockResource(std::string("Gertboard Expansion Port ") + this->opdid->to_string(pinNumber), nodeName);
			
			// setup the port instance and add it; use internal pin number
			DigitalExpansionPort *port = new DigitalExpansionPort(abstractOPDID, this, nodeName.c_str(), pinNumber);
			// set default group: Gertboard's node's group
			port->setGroup(group);

			// evaluate driver type (important: before regular configuration which may set output mode and state)
			std::string driverType = portConfig->getString("DriverType", "");
			if (driverType == "Standard") {
				port->driverType = DigitalExpansionPort::STANDARD;
			} else
			if (driverType == "LowSide") {
				port->driverType = DigitalExpansionPort::LOW_SIDE;
			} else
			if (driverType == "HighSide") {
				port->driverType = DigitalExpansionPort::HIGH_SIDE;
			} else
			if (driverType != "")
				throw Poco::DataException("Invalid value for DriverType: Expected 'Standard', 'LowSide' or 'HighSide': " + driverType);

			abstractOPDID->configureDigitalPort(portConfig, port);

			abstractOPDID->addPort(port);
		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		nli++;
	}
	
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("GertboardPlugin setup completed successfully as node " + node);
}

void GertboardPlugin::masterConnected() {
}

void GertboardPlugin::masterDisconnected() {
}

void GertboardPlugin::sendExpansionPortCode(uint8_t code){
	if (uart0_filestream != -1) {
		if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
			this->opdid->log(this->nodeID + ": Sending expansion port control code: " + this->opdid->to_string((int)code));

		// flush the input buffer
		uint8_t rx_buffer[1];
		while (read(uart0_filestream, (void*)rx_buffer, 1) > 0);
	
		int count = write(uart0_filestream, &code, 1);
		if (count < 0)
			throw OPDI_Port::PortError("Serial communication error while sending: " + this->opdid->to_string(errno));
	} else
		throw OPDI_Port::PortError("Serial communication not initialized");
}

uint8_t GertboardPlugin::receiveExpansionPortCode(void) {
	if (uart0_filestream != -1) {
		uint8_t rx_buffer[1];
		uint64_t ticks = opdi_get_time_ms();
		
		int rx_length = 0;

		while ((rx_length <= 0) && (opdi_get_time_ms() - ticks < this->serialTimeoutMs)) {
			rx_length = read(uart0_filestream, (void*)rx_buffer, 1);
		}
		if (rx_length < 0)
			throw OPDI_Port::PortError("Serial communication error while receiving: " + this->opdid->to_string(errno));
		else if (rx_length == 0)
			throw OPDI_Port::PortError("Serial communication timeout");

		if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
			this->opdid->log(this->nodeID + ": Received expansion port return code: " + this->opdid->to_string((int)rx_buffer[0]));
		return rx_buffer[0];
	} else
		throw OPDI_Port::PortError("Serial communication not initialized");
}

// plugin factory function
extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the GertboardPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new GertboardPlugin();
}

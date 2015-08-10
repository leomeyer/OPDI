#include <bitset>
#define _USE_MATH_DEFINES // for C++
#include <math.h>

#include "Poco/Tuple.h"
#include "Poco/Timezone.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/NumberParser.h"
#include "Poco/Format.h"

#include "ctb-0.16/ctb.h"

#include "opdi_port.h"
#include "opdi_platformfuncs.h"

#include "OPDID_Ports.h"

///////////////////////////////////////////////////////////////////////////////
// Logic Port
///////////////////////////////////////////////////////////////////////////////

OPDID_LogicPort::OPDID_LogicPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {

	this->opdid = opdid;
	this->function = UNKNOWN;
	this->funcN = -1;
	this->negate = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the line to an invalid state
	// this will trigger the detection logic in the first call of doWork
	// so that all dependent output ports will be set to a defined state
	this->line = -1;
}

OPDID_LogicPort::~OPDID_LogicPort() {
}

void OPDID_LogicPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	std::string function = config->getString("Function", "OR");

	try {
		if (function == "OR")
			this->function = OR;
		else
		if (function == "AND")
			this->function = AND;
		else
		if (function == "XOR")
			this->function = XOR;
		else {
			std::string prefix("ATLEAST");
			if (!function.compare(0, prefix.size(), prefix)) {
				this->function = ATLEAST;
				this->funcN = atoi(function.substr(prefix.size() + 1).c_str());
			} else {
				std::string prefix("ATMOST");
				if (!function.compare(0, prefix.size(), prefix)) {
					this->function = ATMOST;
					this->funcN = atoi(function.substr(prefix.size() + 1).c_str());
				} else {
					std::string prefix("EXACT");
					if (!function.compare(0, prefix.size(), prefix)) {
						this->function = EXACT;
						this->funcN = atoi(function.substr(prefix.size() + 1).c_str());
					}
				}
			}
		}
	} catch (...) {
		throw Poco::DataException(std::string("Syntax error for LogicPort ") + this->getID() + ", Function setting: Expected OR, AND, XOR, ATLEAST <n>, ATMOST <n> or EXACT <n>");
	}

	if (this->function == UNKNOWN)
		throw Poco::DataException("Unknown function specified for LogicPort: " + function);
	if ((this->function == ATLEAST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATLEAST of LogicPort: " + function);
	if ((this->function == ATMOST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATMOST of LogicPort: " + function);
	if ((this->function == EXACT) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function EXACT of LogicPort: " + function);

	this->negate = config->getBool("Negate", false);

	this->inputPortStr = config->getString("InputPorts", "");
	if (this->inputPortStr == "")
		throw Poco::DataException("Expected at least one input port for LogicPort");

	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");
}

void OPDID_LogicPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setLine(uint8_t line) {
	throw PortError(std::string(this->getID()) + ": The line of a LogicPort cannot be set directly");
}

void OPDID_LogicPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	if (this->function == UNKNOWN)
		throw PortError(std::string("Logic function is unknown; port not configured correctly: ") + this->id);

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "InputPorts", this->inputPortStr, this->inputPorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);
}

uint8_t OPDID_LogicPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// count how many input ports are High
	size_t highCount = 0;
	DigitalPortList::iterator it = this->inputPorts.begin();
	while (it != this->inputPorts.end()) {
		uint8_t mode;
		uint8_t line;
		try {
			(*it)->getState(&mode, &line);
		} catch (Poco::Exception &e) {
			this->opdid->logNormal(std::string("Error querying port ") + (*it)->getID() + ": " + e.message());
		}
		highCount += line;
		it++;
	}

	// evaluate function
	uint8_t newLine = (this->negate ? 1 : 0);

	switch (this->function) {
	case UNKNOWN:
		return OPDI_STATUS_OK;
	case OR: if (highCount > 0)
				 newLine = (this->negate ? 0 : 1);
		break;
	case AND: if (highCount == this->inputPorts.size())
				  newLine = (this->negate ? 0 : 1);
		break;
	// XOR is implemented as modulo; meaning an odd number of inputs must be high
	case XOR: if (highCount % 2 == 1)
				newLine = (this->negate ? 0 : 1);
		break;
	case ATLEAST: if (highCount >= this->funcN)
					  newLine = (this->negate ? 0 : 1);
		break;
	case ATMOST: if (highCount <= this->funcN)
					 newLine = (this->negate ? 0 : 1);
		break;
	case EXACT: if (highCount == this->funcN)
					 newLine = (this->negate ? 0 : 1);
		break;
	}

	// change detected?
	if (newLine != this->line) {
		this->logDebug(this->ID() + ": Detected line change (" + this->to_string(highCount) + " of " + this->to_string(this->inputPorts.size()) + " inputs port are High)");

		OPDI_DigitalPort::setLine(newLine);

		// regular output ports
		DigitalPortList::iterator it = this->outputPorts.begin();
		while (it != this->outputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line != newLine) {
					this->logDebug(this->ID() + ": Changing line of port " + (*it)->getID() + " to " + (newLine == 0 ? "Low" : "High"));
					(*it)->setLine(newLine);
				}
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string("Error changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line == newLine) {
					this->logDebug(this->ID() + ": Changing line of inverse port " + (*it)->getID() + " to " + (newLine == 0 ? "High" : "Low"));
					(*it)->setLine((newLine == 0 ? 1 : 0));
				}
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string("Error changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Pulse Port
///////////////////////////////////////////////////////////////////////////////

OPDID_PulsePort::OPDID_PulsePort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {

	this->opdid = opdid;
	this->negate = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the pulse state to an invalid state
	// this will trigger the detection logic in the first call of doWork
	// so that all dependent output ports will be set to a defined state
	this->pulseState = -1;
	this->lastStateChangeTime = 0;
	this->disabledState = -1;
}

OPDID_PulsePort::~OPDID_PulsePort() {
}

void OPDID_PulsePort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->negate = config->getBool("Negate", false);

	std::string portLine = config->getString("Line", "");
	if (portLine == "High") {
		this->setLine(1);
	} else if (portLine == "Low") {
		this->setLine(0);
	} else if (portLine != "")
		throw Poco::DataException("Unknown Line specified; expected 'Low' or 'High'", portLine);

	std::string disabledState = config->getString("DisabledState", "");
	if (disabledState == "High") {
		this->disabledState = 1;
	} else if (disabledState == "Low") {
		this->disabledState = 0;
	} else if (disabledState != "")
		throw Poco::DataException("Unknown DisabledState specified; expected 'Low' or 'High'", disabledState);

	this->enablePortStr = config->getString("EnablePorts", "");
	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");

	this->period.initialize(this, "Period", config->getString("Period", "-1"));
	if (!this->period.validateAsInt(0, INT_MAX))
		throw Poco::DataException("Specify a positive integer value for the Period setting of a PulsePort: " + this->to_string(this->period));

	// duty cycle is specified in percent
	this->dutyCycle.initialize(this, "DutyCycle", config->getString("DutyCycle", "50"));
	if (!this->dutyCycle.validate(0, 100))
		throw Poco::DataException("Specify a percentage value from 0 - 100 for the DutyCycle setting of a PulsePort: " + this->to_string(this->dutyCycle));
}

void OPDID_PulsePort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a PulsePort cannot be changed");
}

void OPDID_PulsePort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "EnablePorts", this->enablePortStr, this->enablePorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);
}

uint8_t OPDID_PulsePort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// default: pulse is enabled if the line is High
	bool enabled = (this->line == 1);

	if (!enabled  && (this->enablePorts.size() > 0)) {
		int highCount = 0;
		DigitalPortList::iterator it = this->enablePorts.begin();
		while (it != this->enablePorts.end()) {
			uint8_t mode;
			uint8_t line;
			try {
				(*it)->getState(&mode, &line);
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string(this->getID()) + ": Error querying port " + (*it)->getID() + ": " + e.message());
			}
			highCount += line;
			it++;
			if (highCount > 0)
				break;
		}
		// pulse is enabled if there is at least one EnablePort with High
		enabled |= highCount > 0;
	}

	int32_t period = static_cast<int32_t>(this->period);
	if (period < 0) {
		this->logWarning(this->ID() + ": Period may not be negative: " + to_string(period));
		return OPDI_STATUS_OK;
	}

	double dutyCycle = this->dutyCycle;
	if (dutyCycle < 0) {
		this->logWarning(this->ID() + ": DutyCycle may not be negative: " + to_string(dutyCycle));
		return OPDI_STATUS_OK;
	}
	if (dutyCycle > 100) {
		this->logWarning(this->ID() + ": DutyCycle may not exceed 100%: " + to_string(dutyCycle));
		return OPDI_STATUS_OK;
	}

	// determine new line level
	uint8_t newState = this->pulseState;

	if (enabled) {
		// check whether the time for state change has been reached
		uint64_t timeDiff = opdi_get_time_ms() - this->lastStateChangeTime;
		// current state (logical) Low?
		if (this->pulseState == (this->negate ? 1 : 0)) {
			// time up to High reached?
			if (timeDiff > period * (1.0 - dutyCycle / 100.0)) 
				// switch to (logical) High
				newState = (this->negate ? 0 : 1);
		} else {
			// time up to Low reached?
			if (timeDiff > period * dutyCycle / 100.0)
				// switch to (logical) Low
				newState = (this->negate ? 1 : 0);
		}
	} else {
		// if the port is not enabled, and an inactive state is specified, set it
		if (this->disabledState > -1)
			newState = this->disabledState;
	}

	// evaluate function

	// change detected?
	if (newState != this->pulseState) {
		this->logDebug(this->ID() + ": Changing pulse to " + (newState == 1 ? "High" : "Low") + " (dTime: " + to_string(opdi_get_time_ms() - this->lastStateChangeTime) + " ms)");

		this->lastStateChangeTime = opdi_get_time_ms();

		// set the new state
		this->pulseState = newState;

		DigitalPortList::iterator it = this->outputPorts.begin();
		// regular output ports
		while (it != this->outputPorts.end()) {
			try {
				(*it)->setLine(newState);
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string(this->getID()) + ": Error setting output port state: " + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				(*it)->setLine((newState == 0 ? 1 : 0));
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string(this->getID()) + ": Error setting inverse output port state: " + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Selector Port
///////////////////////////////////////////////////////////////////////////////

OPDID_SelectorPort::OPDID_SelectorPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {

	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the line to an invalid state
	this->line = -1;
}

OPDID_SelectorPort::~OPDID_SelectorPort() {
}

void OPDID_SelectorPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->selectPortStr = config->getString("SelectPort", "");
	if (this->selectPortStr == "")
		throw Poco::DataException("You have to specify the SelectPort");

	int pos = config->getInt("Position", -1);
	if ((pos < 0) || (pos > 65535))
		throw Poco::DataException("You have to specify a SelectPort position that is greater than -1 and lower than 65536");

	this->position = pos;
}

void OPDID_SelectorPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a SelectorPort cannot be changed");
}

void OPDID_SelectorPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a SelectorPort cannot be changed");
}

void OPDID_SelectorPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);
	if (this->line == 1) {
		// set the specified select port to the specified position
		this->selectPort->setPosition(this->position);
	}
}

void OPDID_SelectorPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find port; throws errors if something required is missing
	this->selectPort = this->findSelectPort(this->getID(), "SelectPort", this->selectPortStr, true);

	// check position range
	if (this->position > this->selectPort->getMaxPosition())
		throw Poco::DataException(std::string(this->getID()) + ": The specified selector position exceeds the maximum of port " + this->selectPort->getID() + ": " + to_string(this->selectPort->getMaxPosition()));
}

uint8_t OPDID_SelectorPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// check whether the select port is in the specified position
	uint16_t pos;
	this->selectPort->getState(&pos);
	if (pos == this->position) {
		if (this->line != 1) {
			this->logDebug(this->ID() + ": Port " + this->selectPort->getID() + " is in position " + to_string(this->position) + ", switching SelectorPort to High");
			OPDI_DigitalPort::setLine(1);
		}
	} else {
		if (this->line != 0) {
			this->logDebug(this->ID() + ": Port " + this->selectPort->getID() + " is in position " + to_string(this->position) + ", switching SelectorPort to Low");
			OPDI_DigitalPort::setLine(0);
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Error Detector Port
///////////////////////////////////////////////////////////////////////////////

OPDID_ErrorDetectorPort::OPDID_ErrorDetectorPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_INPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_INPUT_FLOATING);

	// make sure to trigger state change at first doWork
	this->line = -1;
}

OPDID_ErrorDetectorPort::~OPDID_ErrorDetectorPort() {
}

void OPDID_ErrorDetectorPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configureDigitalPort(config, this);	
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->inputPortStr = this->opdid->getConfigString(config, "InputPorts", "", true);
	this->negate = config->getBool("Negate", false);
}

void OPDID_ErrorDetectorPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of an ErrorDetectorPort cannot be changed");
}

void OPDID_ErrorDetectorPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of an ErrorDetectorPort cannot be changed");
}

void OPDID_ErrorDetectorPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "InputPorts", this->inputPortStr, this->inputPorts);
}

uint8_t OPDID_ErrorDetectorPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	int8_t newState = 0;

	// if any port has an error, set the line state to 1
	OPDI::PortList::iterator it = this->inputPorts.begin();
	while (it != this->inputPorts.end()) {
		if ((*it)->hasError()) {
			this->logExtreme(this->ID() + ": Detected error on port: " + (*it)->getID());
			newState = 1;
			break;
		}
		it++;
	}

	if (negate)
		newState = (newState == 0 ? 1 : 0);

	// change?
	if (this->line != newState) {
		this->logDebug(this->ID() + ": Changing line state to: " + (newState == 1 ? "High" : "Low"));
		this->line = newState;
		this->doSelfRefresh();
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Serial Streaming Port
///////////////////////////////////////////////////////////////////////////////

OPDID_SerialStreamingPort::OPDID_SerialStreamingPort(AbstractOPDID *opdid, const char *id) : OPDI_StreamingPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->mode = PASS_THROUGH;
	this->device = NULL;
	this->serialPort = new ctb::SerialPort();
}

OPDID_SerialStreamingPort::~OPDID_SerialStreamingPort() {
}

uint8_t OPDID_SerialStreamingPort::doWork(uint8_t canSend)  {
	OPDI_StreamingPort::doWork(canSend);

	if (this->device == NULL)
		return OPDI_STATUS_OK;

	if (this->mode == LOOPBACK) {
		// byte available?
		if (this->available(0) > 0) {
			char result;
			if (this->read(&result) > 0) {
				this->logDebug(this->ID() + ": Looping back received serial data byte: " + this->opdid->to_string((int)result));

				// echo
				this->write(&result, 1);
			}
		}
	}

	return OPDI_STATUS_OK;
}

void OPDID_SerialStreamingPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configureStreamingPort(config, this);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	std::string serialPortName = this->opdid->getConfigString(config, "SerialPort", "", true);
	int baudRate = config->getInt("BaudRate", 9600);
	std::string protocol = config->getString("Protocol", "8N1");
	// int timeout = config->getInt("Timeout", 100);

	this->logVerbose(this->ID() + ": Opening serial port " + serialPortName + " with " + this->opdid->to_string(baudRate) + " baud and protocol " + protocol);

	// try to lock the port name as a resource
	this->opdid->lockResource(serialPortName, this->getID());

	if (this->serialPort->Open(serialPortName.c_str(), baudRate, 
							protocol.c_str(), 
							ctb::SerialPort::NoFlowControl) >= 0) {
		this->device = this->serialPort;
	} else {
		throw Poco::ApplicationException(std::string(this->getID()) + ": Unable to open serial port: " + serialPortName);
	}

	this->logVerbose(this->ID() + ": Serial port " + serialPortName + " opened successfully");

	std::string modeStr = config->getString("Mode", "");
	if (modeStr == "Loopback") {
		this->mode = LOOPBACK;
	} else
	if (modeStr == "Passthrough") {
		this->mode = PASS_THROUGH;
	} else
		if (modeStr != "")
			throw Poco::DataException(std::string(this->getID()) + ": Invalid mode specifier; expected 'Passthrough' or 'Loopback': " + modeStr);
}

int OPDID_SerialStreamingPort::write(char *bytes, size_t length) {
	return this->device->Write(bytes, length);
}

int OPDID_SerialStreamingPort::available(size_t count) {
	// count has no meaning in this implementation

	char buf;
	int code = this->device->Read(&buf, 1);
	// error?
	if (code < 0)
		return code;
	// byte read?
	if (code > 0) {
		this->device->PutBack(buf);
		return 1;
	}
	// nothing available
	return 0;
}

int OPDID_SerialStreamingPort::read(char *result) {
	char buf;
	int code = this->device->Read(&buf, 1);
	// error?
	if (code < 0)
		return code;
	// byte read?
	if (code > 0) {
		*result = buf;
		return 1;
	}
	// nothing available
	return 0;
}

bool OPDID_SerialStreamingPort::hasError(void) {
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Logger Streaming Port
///////////////////////////////////////////////////////////////////////////////

OPDID_LoggerPort::OPDID_LoggerPort(AbstractOPDID *opdid, const char *id) : OPDI_StreamingPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->logPeriod = 10000;		// default: 10 seconds
	this->lastEntryTime = 0;
	this->format = CSV;
	this->separator = ";";
}

OPDID_LoggerPort::~OPDID_LoggerPort() {
}

std::string OPDID_LoggerPort::getPortStateStr(OPDI_Port* port) {
	return this->opdid->getPortStateStr(port);
}

void OPDID_LoggerPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_StreamingPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "Ports", this->portsToLogStr, this->portsToLog);
}

uint8_t OPDID_LoggerPort::doWork(uint8_t canSend)  {
	OPDI_StreamingPort::doWork(canSend);

	// check whether the time for a new entry has been reached
	uint64_t timeDiff = opdi_get_time_ms() - this->lastEntryTime;
	if (timeDiff < this->logPeriod)
		return OPDI_STATUS_OK;

	// first time writing? write a header
	bool writeHeader = (lastEntryTime == 0);

	this->lastEntryTime = opdi_get_time_ms();

	// build log entry
	std::string entry;

	if (format == CSV) {
		if (writeHeader) {
			entry = "Timestamp" + this->separator;
			// go through port list, build header
			OPDI::PortList::iterator it = this->portsToLog.begin();
			while (it != this->portsToLog.end()) {
				entry += (*it)->getID();
				// separator necessary?
				if (it != this->portsToLog.end() - 1) 
					entry += this->separator;
				it++;
			}
			this->outFile << entry << std::endl;
		}
		entry = this->opdid->getTimestampStr() + this->separator;
		// go through port list
		OPDI::PortList::iterator it = this->portsToLog.begin();
		while (it != this->portsToLog.end()) {
			entry += this->getPortStateStr(*it);
			// separator necessary?
			if (it != this->portsToLog.end() - 1) 
				entry += this->separator;
			it++;
		}
	}

	if (!this->outFile.is_open())
		return OPDI_STATUS_OK;

	// write to output
	this->outFile << entry << std::endl;

	return OPDI_STATUS_OK;
}

void OPDID_LoggerPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configureStreamingPort(config, this);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->logPeriod = config->getInt("Period", this->logPeriod);
	this->separator = config->getString("Separator", this->separator);

	std::string formatStr = config->getString("Format", "CSV");
	if (formatStr != "CSV")
		throw Poco::DataException(std::string(this->getID()) + ": Formats other than CSV are currently not supported");

	std::string outFileStr = config->getString("OutputFile", "");
	if (outFileStr != "") {
		// try to lock the output file name as a resource
		this->opdid->lockResource(outFileStr, this->getID());

		this->logVerbose(this->ID() + ": Opening output log file " + outFileStr);

		// open the stream in append mode
		this->outFile.open(outFileStr, std::ios_base::app);
	} else
		throw Poco::DataException(std::string(this->getID()) + ": The OutputFile setting must be specified");

	this->portsToLogStr = this->opdid->getConfigString(config, "Ports", "", true);
}

int OPDID_LoggerPort::write(char *bytes, size_t length) {
	return 0;
}

int OPDID_LoggerPort::available(size_t count) {
	// count has no meaning in this implementation
	// nothing available
	return 0;
}

int OPDID_LoggerPort::read(char *result) {
	// nothing available
	return 0;
}

bool OPDID_LoggerPort::hasError(void) {
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Fader Port
///////////////////////////////////////////////////////////////////////////////

OPDID_FaderPort::OPDID_FaderPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->mode = LINEAR;
	this->lastValue = -1;
	this->invert = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
}

OPDID_FaderPort::~OPDID_FaderPort() {
}

void OPDID_FaderPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	std::string modeStr = config->getString("FadeMode", "");
	if (modeStr == "Linear") 
		this->mode = LINEAR;
	else if (modeStr == "Exponential")
		this->mode = EXPONENTIAL;
	else if (modeStr != "")
		throw Poco::DataException(this->ID() + ": Invalid FadeMode setting specified; expected 'Linear' or 'Exponential': " + modeStr);

	this->leftValue.initialize(this, "Left", config->getString("Left", "-1"));
	if (!this->leftValue.validate(0.0, 100.0))
		throw Poco::DataException(this->ID() + ": Value for 'Left' must be between 0 and 100 percent");
	this->rightValue.initialize(this, "Right", config->getString("Right", "-1"));
	if (!this->rightValue.validate(0.0, 100.0))
		throw Poco::DataException(this->ID() + ": Value for 'Right' must be between 0 and 100 percent");
	this->durationMsValue.initialize(this, "Duration", config->getString("Duration", "-1"));
	if (!this->durationMsValue.validateAsInt(0, INT_MAX))
		throw Poco::DataException(this->ID() + ": 'Duration' must be a positive non-zero value (in milliseconds)");

	if (this->mode == EXPONENTIAL) {
		this->expA = config->getDouble("ExpA", 1);
		if (this->expA < 0.0)
			throw Poco::DataException(this->ID() + ": Value for 'ExpA' must be greater than 0 (default: 1)");
		this->expB = config->getDouble("ExpB", 10);
		if (this->expB < 0.0)
			throw Poco::DataException(this->ID() + ": Value for 'ExpB' must be greater than 0 (default: 10)");

		// determine maximum result of the exponentiation, depending on the coefficients expA and expB
		this->expMax = this->expA * (exp(this->expB) - 1);
	}
	this->invert = config->getBool("Invert", this->invert);

	this->outputPortStr = opdid->getConfigString(config, "OutputPorts", "", true);

	this->opdid->configurePort(config, this, 0);
}

void OPDID_FaderPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a FaderPort cannot be changed");
}

void OPDID_FaderPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a FaderPort cannot be changed");
}

void OPDID_FaderPort::setLine(uint8_t line) {
	if (line == 1) {
		// store current values on start (might be resolved by ValueResolvers, and we don't want them to change during fading
		// because each value might again refer to the output port - not an uncommon scenario for e.g. dimmers)
		this->left = this->leftValue;
		this->right = this->rightValue;
		this->durationMs = this->durationMsValue;

		// don't fade if the duration is impracticably low
		if (this->durationMs < 5) {
			this->logVerbose(this->ID() + ": Refusing to fade because duration is impracticably low: " + to_string(this->durationMs));
		} else {
			OPDI_DigitalPort::setLine(line);
			this->startTime = Poco::Timestamp();
			// cause correct log output
			this->lastValue = -1;
			this->logVerbose(this->ID() + ": Start fading at " + to_string(this->left) + "% with a duration of " + to_string(this->durationMs) + " ms");
		}
	} else {
		OPDI_DigitalPort::setLine(line);
		this->logVerbose(this->ID() + ": Stopped fading at " + to_string(this->lastValue * 100.0) + "%");
	}
}

void OPDID_FaderPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
}

uint8_t OPDID_FaderPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 1) {

		if (this->durationMs < 0) {
			this->logWarning(this->ID() + ": Duration may not be negative; disabling fader: " + to_string(this->durationMs));
			// disable the fader immediately
			OPDI_DigitalPort::setLine(0);
			this->refreshRequired = true;
			return OPDI_STATUS_OK;
		}

		// calculate time difference
		Poco::Timestamp now;
		Poco::Timestamp::TimeVal elapsedMs = (now.epochMicroseconds() - this->startTime.epochMicroseconds()) / 1000;

		// end reached?
		if (elapsedMs > this->durationMs) {
			this->setLine(0);
			this->refreshRequired = true;
			return OPDI_STATUS_OK;
		}

		// calculate current value (linear first) within the range [0, 1]
		double value = 0.0;
		if (this->mode == LINEAR) {
			if (this->invert)
				value = (this->right - (double)elapsedMs / (double)this->durationMs * (this->right - this->left)) / 100.0;
			else
				value = (this->left + (double)elapsedMs / (double)this->durationMs * (this->right - this->left)) / 100.0;
		} else
		if (this->mode == EXPONENTIAL) {
			// calculate exponential value; start with value relative to the range
			if (this->invert)
				value = 1.0 - (double)elapsedMs / (double)this->durationMs;
			else
				value = (double)elapsedMs / (double)this->durationMs;

			// exponentiate value; map within [0..1]
			value = (this->expMax <= 0 ? 1 : (this->expA * (exp(this->expB * value)) - 1) / this->expMax);

			if (value < 0.0)
				value = 0.0;

			// map back to the target range
			value = (this->left + value * (this->right - this->left)) / 100.0;
		} else
			value = 0.0;

		this->logExtreme(this->ID() + ": Setting current fader value to " + to_string(value * 100.0) + "%");

		// regular output ports
		PortList::iterator it = this->outputPorts.begin();
		while (it != this->outputPorts.end()) {
			try {
				if (!strcmp((*it)->getType(), OPDI_PORTTYPE_ANALOG)) {
					((OPDI_AnalogPort*)(*it))->setRelativeValue(value);
				} else
				if (!strcmp((*it)->getType(), OPDI_PORTTYPE_DIAL)) {
					OPDI_DialPort* port = (OPDI_DialPort*)(*it);
					double pos = port->getMin() + (port->getMax() - port->getMin()) * value;
					port->setPosition((int64_t)pos);
				} else
					throw Poco::Exception("The port " + (*it)->ID() + " is neither an AnalogPort nor a DialPort");
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(this->ID() + ": Error changing port " + (*it)->getID() + ": " + e.message());
			}
			it++;
		}

		this->lastValue = value;
	}

	return OPDI_STATUS_OK;
}

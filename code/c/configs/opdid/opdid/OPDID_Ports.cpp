#include <bitset>
#define _USE_MATH_DEFINES // for C++
#include <math.h>
#include <numeric>
#include <functional>

#include "Poco/String.h"
#include "Poco/Tuple.h"
#include "Poco/Timezone.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/NumberParser.h"
#include "Poco/Format.h"
#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"
#include "Poco/ScopedLock.h"
#include "Poco/FileStream.h"

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

void OPDID_LogicPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setMode(uint8_t /*mode*/) {
	throw PortError(this->ID() + ": The mode of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setLine(uint8_t /*line*/) {
	throw PortError(this->ID() + ": The line of a LogicPort cannot be set directly");
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
		++it;
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
			++it;
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
			++it;
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
	if (!this->period.validate(0, INT_MAX))
		throw Poco::DataException("Specify a positive integer value for the Period setting of a PulsePort: " + this->to_string(this->period.value()));

	// duty cycle is specified in percent
	this->dutyCycle.initialize(this, "DutyCycle", config->getString("DutyCycle", "50"));
	if (!this->dutyCycle.validate(0, 100))
		throw Poco::DataException("Specify a percentage value from 0 - 100 for the DutyCycle setting of a PulsePort: " + this->to_string(this->dutyCycle.value()));
}

void OPDID_PulsePort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setMode(uint8_t /*mode*/) {
	throw PortError(this->ID() + ": The mode of a PulsePort cannot be changed");
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

	if (!enabled && (this->enablePorts.size() > 0)) {
		int highCount = 0;
		DigitalPortList::iterator it = this->enablePorts.begin();
		while (it != this->enablePorts.end()) {
			uint8_t mode;
			uint8_t line;
			try {
				(*it)->getState(&mode, &line);
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(this->ID() + ": Error querying port " + (*it)->getID() + ": " + e.message());
			}
			highCount += line;
			++it;
			if (highCount > 0)
				break;
		}
		// pulse is enabled if there is at least one EnablePort with High
		enabled |= highCount > 0;
	}

	int32_t period = this->period.value();
	if (period < 0) {
		this->logWarning(this->ID() + ": Period may not be negative: " + to_string(period));
		return OPDI_STATUS_OK;
	}

	double dutyCycle = this->dutyCycle.value();
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
				this->opdid->logNormal(this->ID() + ": Error setting output port state: " + (*it)->getID() + ": " + e.message());
			}
			++it;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				(*it)->setLine((newState == 0 ? 1 : 0));
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(this->ID() + ": Error setting inverse output port state: " + (*it)->getID() + ": " + e.message());
			}
			++it;
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

void OPDID_SelectorPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of a SelectorPort cannot be changed");
}

void OPDID_SelectorPort::setMode(uint8_t /*mode*/) {
	throw PortError(this->ID() + ": The mode of a SelectorPort cannot be changed");
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
		throw Poco::DataException(this->ID() + ": The specified selector position exceeds the maximum of port " + this->selectPort->getID() + ": " + to_string(this->selectPort->getMaxPosition()));
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

void OPDID_ErrorDetectorPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of an ErrorDetectorPort cannot be changed");
}

void OPDID_ErrorDetectorPort::setMode(uint8_t /*mode*/) {
	throw PortError(this->ID() + ": The mode of an ErrorDetectorPort cannot be changed");
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
		++it;
	}

	if (negate)
		newState = (newState == 0 ? 1 : 0);

	// change?
	if (this->line != newState) {
		this->logDebug(this->ID() + ": Changing line state to: " + (newState == 1 ? "High" : "Low"));
		this->line = newState;
		this->doRefresh();
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Serial Streaming Port
///////////////////////////////////////////////////////////////////////////////

OPDID_SerialStreamingPort::OPDID_SerialStreamingPort(AbstractOPDID *opdid, const char *id) : OPDI_StreamingPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->mode = PASS_THROUGH;
	this->device = nullptr;
	this->serialPort = new ctb::SerialPort();
}

OPDID_SerialStreamingPort::~OPDID_SerialStreamingPort() {
}

uint8_t OPDID_SerialStreamingPort::doWork(uint8_t canSend)  {
	OPDI_StreamingPort::doWork(canSend);

	if (this->device == nullptr)
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
		throw Poco::ApplicationException(this->ID() + ": Unable to open serial port: " + serialPortName);
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
			throw Poco::DataException(this->ID() + ": Invalid mode specifier; expected 'Passthrough' or 'Loopback': " + modeStr);
}

int OPDID_SerialStreamingPort::write(char *bytes, size_t length) {
	return this->device->Write(bytes, length);
}

int OPDID_SerialStreamingPort::available(size_t /*count*/) {
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

bool OPDID_SerialStreamingPort::hasError(void) const {
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Logger Streaming Port
///////////////////////////////////////////////////////////////////////////////

OPDID_LoggerPort::OPDID_LoggerPort(AbstractOPDID *opdid, const char *id) : OPDI_StreamingPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->logPeriod = 10000;		// default: 10 seconds
	this->writeHeader = true;
	this->lastEntryTime = opdi_get_time_ms();		// wait until writing first record
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

	this->lastEntryTime = opdi_get_time_ms();

	// build log entry
	std::string entry;

	if (format == CSV) {
		if (this->writeHeader) {
			entry = "Timestamp" + this->separator;
			// go through port list, build header
			OPDI::PortList::iterator it = this->portsToLog.begin();
			while (it != this->portsToLog.end()) {
				entry += (*it)->getID();
				// separator necessary?
				if (it != this->portsToLog.end() - 1) 
					entry += this->separator;
				++it;
			}
			this->outFile << entry << std::endl;
			this->writeHeader = false;
		}
		entry = this->opdid->getTimestampStr() + this->separator;
		// go through port list
		OPDI::PortList::iterator it = this->portsToLog.begin();
		while (it != this->portsToLog.end()) {
			entry += this->getPortStateStr(*it);
			// separator necessary?
			if (it != this->portsToLog.end() - 1) 
				entry += this->separator;
			++it;
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
		throw Poco::DataException(this->ID() + ": Formats other than CSV are currently not supported");

	std::string outFileStr = config->getString("OutputFile", "");
	if (outFileStr != "") {
		// try to lock the output file name as a resource
		this->opdid->lockResource(outFileStr, this->getID());

		this->logVerbose(this->ID() + ": Opening output log file " + outFileStr);

		// open the stream in append mode
		this->outFile.open(outFileStr, std::ios_base::app);
	} else
		throw Poco::DataException(this->ID() + ": The OutputFile setting must be specified");

	this->portsToLogStr = this->opdid->getConfigString(config, "Ports", "", true);
}

int OPDID_LoggerPort::write(char * /*bytes*/, size_t /*length*/) {
	return 0;
}

int OPDID_LoggerPort::available(size_t /*count*/) {
	// count has no meaning in this implementation
	// nothing available
	return 0;
}

int OPDID_LoggerPort::read(char * /*result*/) {
	// nothing available
	return 0;
}

bool OPDID_LoggerPort::hasError(void) const {
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
	this->switchOffAction = NONE;
	this->actionToPerform = NONE;

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
	if (!this->durationMsValue.validate(0, INT_MAX))
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
	std::string switchOffActionStr = config->getString("SwitchOffAction", "None");
	if (switchOffActionStr == "SetToLeft") {
		this->switchOffAction = SET_TO_LEFT;
	} else
	if (switchOffActionStr == "SetToRight") {
		this->switchOffAction = SET_TO_RIGHT;
	} else
	if (switchOffActionStr == "None") {
		this->switchOffAction = NONE;
	} else
		throw Poco::DataException(this->ID() + ": Illegal value for 'SwitchOffAction', expected 'SetToLeft', 'SetToRight', or 'None': " + switchOffActionStr);

	this->outputPortStr = opdid->getConfigString(config, "OutputPorts", "", true);
	this->endSwitchesStr = opdid->getConfigString(config, "EndSwitches", "", false);

	this->opdid->configurePort(config, this, 0);
}

void OPDID_FaderPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of a FaderPort cannot be changed");
}

void OPDID_FaderPort::setMode(uint8_t /*mode*/) {
	throw PortError(this->ID() + ": The mode of a FaderPort cannot be changed");
}

void OPDID_FaderPort::setLine(uint8_t line) {
	uint8_t oldline = this->line;
	if ((oldline == 0) && (line == 1)) {
		// store current values on start (might be resolved by ValueResolvers, and we don't want them to change during fading
		// because each value might again refer to the output port - not an uncommon scenario for e.g. dimmers)
		this->left = this->leftValue.value();
		this->right = this->rightValue.value();
		this->durationMs = this->durationMsValue.value();

		// don't fade if the duration is impracticably low
		if (this->durationMs < 5) {
			this->logDebug(this->ID() + ": Refusing to fade because duration is impracticably low: " + to_string(this->durationMs));
		} else {
			OPDI_DigitalPort::setLine(line);
			this->startTime = Poco::Timestamp();
			// cause correct log output
			this->lastValue = -1;
			this->logDebug(this->ID() + ": Start fading at " + to_string(this->left) + "% with a duration of " + to_string(this->durationMs) + " ms");
		}
	} else {
		OPDI_DigitalPort::setLine(line);
		// switched off?
		if (oldline != this->line) {
			this->logDebug(this->ID() + ": Stopped fading at " + to_string(this->lastValue * 100.0) + "%");
			this->actionToPerform = this->switchOffAction;
			// resolve values again for the switch off action
			this->left = this->leftValue.value();
			this->right = this->rightValue.value();
		}
	}
}

void OPDID_FaderPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->ID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->ID(), "EndSwitches", this->endSwitchesStr, this->endSwitches);
}

uint8_t OPDID_FaderPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// active, or a switch off action needs to be performed?
	if ((this->line == 1) || (this->actionToPerform != NONE)) {
		
		Poco::Timestamp::TimeVal elapsedMs;

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
			elapsedMs = (now.epochMicroseconds() - this->startTime.epochMicroseconds()) / 1000;

			// end reached?
			if (elapsedMs > this->durationMs) {
				this->setLine(0);
				this->refreshRequired = true;

				// set end switches if specified
				DigitalPortList::iterator it = this->endSwitches.begin();
				while (it != this->endSwitches.end()) {
					try {
						this->logDebug(this->ID() + ": Setting line of end switch port " + (*it)->ID() + " to High");
						(*it)->setLine(1);
					} catch (Poco::Exception &e) {
						this->opdid->logWarning(this->ID() + ": Error changing port " + (*it)->getID() + ": " + e.message());
					}
					++it;
				}

				return OPDI_STATUS_OK;
			}
		}

		// calculate current value (linear first) within the range [0, 1]
		double value = 0.0;

		// if the port has been switched off, and a switch action needs to be performed,
		// do it here
		if (this->actionToPerform != NONE) {
			if (this->actionToPerform == SET_TO_LEFT)
				value = this->left / 100.0;
			else
			if (this->actionToPerform == SET_TO_RIGHT)
				value = this->right / 100.0;
			// action has been handled
			this->actionToPerform = NONE;
			this->logDebug(this->ID() + ": Switch off action handled; setting value to: " + this->to_string(value) + "%");
		} else {
			// regular fader operation
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
		}

		this->logExtreme(this->ID() + ": Setting current fader value to " + to_string(value * 100.0) + "%");

		// regular output ports
		PortList::iterator it = this->outputPorts.begin();
		while (it != this->outputPorts.end()) {
			try {
				if (0 == strcmp((*it)->getType(), OPDI_PORTTYPE_ANALOG)) {
					((OPDI_AnalogPort*)(*it))->setRelativeValue(value);
				} else
				if (0 == strcmp((*it)->getType(), OPDI_PORTTYPE_DIAL)) {
					OPDI_DialPort* port = (OPDI_DialPort*)(*it);
					double pos = port->getMin() + (port->getMax() - port->getMin()) * value;
					port->setPosition((int64_t)pos);
				} else
					throw Poco::Exception("The port " + (*it)->ID() + " is neither an AnalogPort nor a DialPort");
			} catch (Poco::Exception& e) {
				this->opdid->logNormal(this->ID() + ": Error changing port " + (*it)->getID() + ": " + e.message());
			}
			++it;
		}

		this->lastValue = value;
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// SceneSelectPort
///////////////////////////////////////////////////////////////////////////////

OPDID_SceneSelectPort::OPDID_SceneSelectPort(AbstractOPDID *opdid, const char *id) : OPDI_SelectPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->positionSet = false;
}

OPDID_SceneSelectPort::~OPDID_SceneSelectPort() {
}

void OPDID_SceneSelectPort::configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig) {
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	// remember configuration file path (scene files are always relative to the configuration file)
	this->configFilePath = config->getString(OPDID_CONFIG_FILE_SETTING);

	// the scene select port requires a prefix or section "<portID>.Scenes"
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> portItems = parentConfig->createView(this->ID() + ".Scenes");

	// get ordered list of items
	Poco::Util::AbstractConfiguration::Keys itemKeys;
	portItems->keys("", itemKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of items (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = itemKeys.begin(); it != itemKeys.end(); ++it) {
		int itemNumber;
		if (!Poco::NumberParser::tryParse(*it, itemNumber) || (itemNumber < 0)) {
			throw Poco::DataException(this->ID() + ": Scene identifiers must be numeric integer values greater or equal than 0; got: " + this->to_string(itemNumber));
		}
		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			++nli;
		}
		Item item(itemNumber, portItems->getString(*it));
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0)
		throw Poco::DataException(this->ID() + ": A scene select port requires at least one scene in its config section", this->ID() + ".Scenes");

	// go through items, create ordered list of char* items
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {
		this->fileList.push_back(nli->get<1>());
		++nli;
	}

	this->opdid->configureSelectPort(config, parentConfig, this, 0);

	// check whether port items and file match in numbers
	if ((uint32_t)(this->getMaxPosition() + 1) != this->fileList.size()) 
		throw Poco::DataException(this->ID() + ": The number of scenes (" + this->to_string(this->fileList.size()) + ")"
			+ " must match the number of items (" + this->to_string(this->getMaxPosition() + 1) + ")");
}

void OPDID_SceneSelectPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_SelectPort::prepare();

	// check files
	FileList::iterator fi = this->fileList.begin();
	while (fi != this->fileList.end()) {
		std::string sceneFile = *fi;

		this->logDebug(this->ID() + ": Checking scene file "+ sceneFile + " relative to configuration file: " + this->configFilePath);

		Poco::Path filePath(this->configFilePath);
		Poco::Path absPath(filePath.absolute());
		Poco::Path parentPath = absPath.parent();
		// append or replace scene file path to path of config file
		Poco::Path finalPath = parentPath.resolve(sceneFile);
		sceneFile = finalPath.toString();
		Poco::File file(finalPath);

		if (!file.exists())
			throw Poco::ApplicationException(this->ID() + ": The scene file does not exist: " + sceneFile);

		// store absolute scene file path
		*fi = sceneFile;

		++fi;
	}
}

uint8_t OPDID_SceneSelectPort::doWork(uint8_t canSend)  {
	OPDI_SelectPort::doWork(canSend);

	// position changed?
	if (this->positionSet) {
		this->logVerbose(this->ID() + ": Scene selected: " + this->getPositionLabel(this->position));

		std::string sceneFile = this->fileList[this->position];

		// prepare scene file parameters (environment, ports, ...)
		std::map<std::string, std::string> parameters;
		this->opdid->getEnvironment(parameters);

		if (this->logVerbosity >= AbstractOPDID::DEBUG) {
			this->logDebug(this->ID() + ": Scene file parameters:");
			std::map<std::string, std::string>::const_iterator it = parameters.begin();
			while (it != parameters.end()) {
				this->logDebug(this->ID() + ":   " + (*it).first + " = " + (*it).second);
				++it;
			}
		}

		// open the config file
		OPDIDConfigurationFile config(sceneFile, parameters);

		// go through sections of the scene file
		Poco::Util::AbstractConfiguration::Keys sectionKeys;
		config.keys("", sectionKeys);

		if (sectionKeys.size() == 0)
			this->logWarning(this->ID() + ": Scene file " + sceneFile + " does not contain any scene information, is this intended?");
		else
			this->logDebug(this->ID() + ": Applying settings from scene file: " + sceneFile);

		for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = sectionKeys.begin(); it != sectionKeys.end(); ++it) {
			// find port corresponding to this section
			OPDI_Port *port = this->opdid->findPortByID((*it).c_str());
			if (port == nullptr)
				this->logWarning(this->ID() + ": In scene file " + sceneFile + ": Port with ID " + (*it) + " not present in current configuration");
			else {
				this->logDebug(this->ID() + ": Applying settings to port: " + *it);

				// configure port according to settings 
				Poco::AutoPtr<Poco::Util::AbstractConfiguration> portConfig = config.createView(*it);

				// configure only state - not the general setup
				try {
					if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
						this->opdid->configureDigitalPort(portConfig, (OPDI_DigitalPort*)port, true);
					} else
					if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
						this->opdid->configureAnalogPort(portConfig, (OPDI_AnalogPort*)port, true);
					} else
					if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
						this->opdid->configureSelectPort(portConfig, &config, (OPDI_SelectPort*)port, true);
					} else
					if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
						this->opdid->configureDialPort(portConfig, (OPDI_DialPort*)port, true);
					} else
						this->logWarning(this->ID() + ": In scene file " + sceneFile + ": Port with ID " + (*it) + " has an unknown type");
				} catch (Poco::Exception &e) {
					this->logWarning(this->ID() + ": In scene file " + sceneFile + ": Error configuring port " + (*it) + ": " + e.message());
				}
			}
		}

		// refresh all ports of a connected master
		this->opdid->refresh(nullptr);

		this->positionSet = false;
	}

	return OPDI_STATUS_OK;
}

void OPDID_SceneSelectPort::setPosition(uint16_t position) {
	OPDI_SelectPort::setPosition(position);

	this->positionSet = true;
}

///////////////////////////////////////////////////////////////////////////////
// FileInputPort
///////////////////////////////////////////////////////////////////////////////

uint8_t OPDID_FileInputPort::doWork(uint8_t canSend) {
	uint8_t result = OPDI_DigitalPort::doWork(canSend);
	if (result != OPDI_STATUS_OK)
		return result;

	Poco::Mutex::ScopedLock(this->mutex);

	// expiry time over?
	if ((this->expiryMs > 0) && (this->lastReloadTime > 0) && (opdi_get_time_ms() - lastReloadTime > (uint64_t)this->expiryMs)) {
		// only if the port's value is ok
		if (this->port->getError() == VALUE_OK) {
			this->logWarning(ID() + ": Value of port '" + this->port->ID() + "' has expired");
			this->port->setError(OPDI_Port::VALUE_EXPIRED);
		}
	}

	if (this->line == 0) {
		// always clear flag if not active (to avoid reloading when set to High)
		this->needsReload = false;
		return OPDI_STATUS_OK;
	}

	// if a delay is specified, ignore reloads until it's up
	if ((this->reloadDelayMs > 0) && (this->lastReloadTime > 0) && (opdi_get_time_ms() - lastReloadTime < (uint64_t)this->reloadDelayMs))
		return OPDI_STATUS_OK;

	if (this->needsReload) {
		this->logDebug(this->ID() + ": Reloading file: " + this->filePath);

		this->lastReloadTime = opdi_get_time_ms();

		this->needsReload = false;

		// read file and parse content
		try {
			std::string content;
			Poco::FileInputStream fis(this->filePath);
			fis >> content;
			fis.close();

			content = Poco::trim(content);
			if (content == "")
				// This case may happen frequently when files are copied.
				// Apparently, on Linux, cp clears the file first or perhaps
				// creates an empty file before filling it with content.
				// To avoid generating too many log warnings, this case
				// is being silently ignored.
				// When the file is being modified, the DirectoryWatcher will
				// hopefully catch this change so data is not lost.
				// So, instead of:
				// throw Poco::DataFormatException("File is empty");
				// do:
				return OPDI_STATUS_OK;

			switch (this->portType) {
			case DIGITAL_PORT: {
				uint8_t line;
				if (content == "0")
					line = 0;
				else
				if (content == "1")
					line = 1;
				else {
					std::string errorContent = (content.length() > 50 ? content.substr(0, 50) + "..." : content);
					throw Poco::DataFormatException("Expected '0' or '1' but got: " + errorContent);
				}
				this->logDebug(this->ID() + ": Setting line of digital port '" + this->port->ID() + "' to " + this->to_string((int)line));
				((OPDI_DigitalPort*)this->port)->setLine(line);
				break;
			}
			case ANALOG_PORT: {
				double value;
				if (!Poco::NumberParser::tryParseFloat(content, value) || (value < 0) || (value > 1)) {
					std::string errorContent = (content.length() > 50 ? content.substr(0, 50) + "..." : content);
					throw Poco::DataFormatException("Expected decimal value between 0 and 1 but got: " + errorContent);
				}
				value = value * this->numerator / this->denominator;
				this->logDebug(this->ID() + ": Setting value of analog port '" + this->port->ID() + "' to " + this->to_string(value));
				((OPDI_AnalogPort*)this->port)->setRelativeValue(value);
				break;
			}
			case DIAL_PORT: {
				int64_t value;
				int64_t min = ((OPDI_DialPort*)this->port)->getMin();
				int64_t max = ((OPDI_DialPort*)this->port)->getMax();
				if (!Poco::NumberParser::tryParse64(content, value)) {
					std::string errorContent = (content.length() > 50 ? content.substr(0, 50) + "..." : content);
					throw Poco::DataFormatException("Expected integer value but got: " + errorContent);
				}
				value = value * this->numerator / this->denominator;
				if ((value < min) || (value > max)) {
					std::string errorContent = (content.length() > 50 ? content.substr(0, 50) + "..." : content);
					throw Poco::DataFormatException("Expected integer value between " + this->to_string(min) + " and " + this->to_string(max) + " but got: " + errorContent);
				}
				this->logDebug(this->ID() + ": Setting position of dial port '" + this->port->ID() + "' to " + this->to_string(value));
				((OPDI_DialPort*)this->port)->setPosition(value);
				break;
			}
			case SELECT_PORT: {
				int32_t value;
				uint16_t min = 0;
				uint16_t max = ((OPDI_SelectPort*)this->port)->getMaxPosition();
				if (!Poco::NumberParser::tryParse(content, value) || (value < min) || (value > max)) {
					std::string errorContent = (content.length() > 50 ? content.substr(0, 50) + "..." : content);
					throw Poco::DataFormatException("Expected integer value between " + this->to_string(min) + " and " + this->to_string(max) + " but got: " + errorContent);
				}
				this->logDebug(this->ID() + ": Setting position of select port '" + this->port->ID() + "' to " + this->to_string(value));
				((OPDI_SelectPort*)this->port)->setPosition(value);
				break;
			}
			default:
				throw Poco::ApplicationException("Port type is unknown or not supported");
			}
		} catch (Poco::Exception &e) {
			this->logWarning(this->ID() + ": Error setting port state from file '" + this->filePath + "': " + e.message());
		}
	}

	return OPDI_STATUS_OK;
}

void OPDID_FileInputPort::fileChangedEvent(const void*, const Poco::DirectoryWatcher::DirectoryEvent& evt) {
	if (evt.item.path() == this->filePath) {
		this->logDebug(this->ID() + ": Detected file modification: " + this->filePath);
		
		Poco::Mutex::ScopedLock(this->mutex);
		this->needsReload = true;
	}
}

OPDID_FileInputPort::OPDID_FileInputPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->directoryWatcher = nullptr;
	this->reloadDelayMs = 0;
	this->expiryMs = 0;
	this->lastReloadTime = 0;
	this->needsReload = false;
	this->numerator = 1;
	this->denominator = 1;
	this->port = nullptr;
	this->portType = UNKNOWN;

	// a FileInput port is presented as an output (being High means that the file input is active)
	this->setDirCaps(OPDI_PORTDIRCAP_OUTPUT);
	// file input is active by default
	this->setLine(1);
}

OPDID_FileInputPort::~OPDID_FileInputPort() {
	if (this->directoryWatcher != nullptr)
		delete this->directoryWatcher;
}

void OPDID_FileInputPort::configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig) {
	this->opdid->configureDigitalPort(config, this);

	this->filePath = opdid->getConfigString(config, "File", "", true);

	// read port node, create configuration view and setup the port according to the specified type
	std::string portNode = opdid->getConfigString(config, "PortNode", "", true);
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = parentConfig->createView(portNode);
	std::string portType = opdid->getConfigString(nodeConfig, "Type", "", true);
	if (portType == "DigitalPort") {
		this->portType = DIGITAL_PORT;
		this->port = new OPDI_DigitalPort(portNode.c_str(), portNode.c_str(), OPDI_PORTDIRCAP_INPUT, 0);
		this->opdid->configureDigitalPort(nodeConfig, (OPDI_DigitalPort*)port);
		// validate setup
		if (((OPDI_DigitalPort*)port)->getMode() != OPDI_DIGITAL_MODE_INPUT_FLOATING)
			throw Poco::DataException(this->ID() + ": Modes other than 'Input' are not supported for a digital file input port: " + portNode);
	} else
	if (portType == "AnalogPort") {
		this->portType = ANALOG_PORT;
		this->port = new OPDI_AnalogPort(portNode.c_str(), portNode.c_str(), OPDI_PORTDIRCAP_INPUT, 0);
		this->opdid->configureAnalogPort(nodeConfig, (OPDI_AnalogPort*)port);
		// validate setup
		if (((OPDI_AnalogPort*)port)->getMode() != OPDI_ANALOG_MODE_INPUT)
			throw Poco::DataException(this->ID() + ": Modes other than 'Input' are not supported for a analog file input port: " + portNode);
	} else
	if (portType == "DialPort") {
		this->portType = DIAL_PORT;
		this->port = new OPDI_DialPort(portNode.c_str());
		this->opdid->configureDialPort(nodeConfig, (OPDI_DialPort*)port);
	} else
	if (portType == "SelectPort") {
		this->portType = SELECT_PORT;
		this->port = new OPDI_SelectPort(portNode.c_str());
		this->opdid->configureSelectPort(nodeConfig, parentConfig, (OPDI_SelectPort*)port);
	} else
	if (portType == "StreamingPort") {
		this->portType = STREAMING_PORT;
		throw Poco::NotImplementedException("Streaming port support not yet implemented");
	} else
		throw Poco::DataException(this->ID() + ": Node " + portNode + ": Type unsupported, expected 'DigitalPort', 'AnalogPort', 'DialPort', 'SelectPort', or 'StreamingPort': " + portType);

	// a file input port is always readonly
	this->port->setReadonly(true);
	this->opdid->addPort(port);

	this->reloadDelayMs = config->getInt("ReloadDelay", 0);
	if (this->reloadDelayMs < 0) {
		throw Poco::DataException(this->ID() + ": If ReloadDelay is specified it must be greater than 0 (ms): " + this->to_string(this->reloadDelayMs));
	}

	this->expiryMs = config->getInt("Expiry", 0);
	if (this->expiryMs < 0) {
		throw Poco::DataException(this->ID() + ": If Expiry is specified it must be greater than 0 (ms): " + this->to_string(this->expiryMs));
	}

	this->numerator = nodeConfig->getInt("Numerator", this->numerator);
	this->denominator = nodeConfig->getInt("Denominator", this->denominator);
	if (this->denominator == 0) 
		throw Poco::InvalidArgumentException(this->ID() + ": The Denominator may not be 0");

	// determine directory and filename
	Poco::Path path(filePath);
	Poco::Path absPath(path.absolute());
	// std::cout << absPath << std::endl;
	
	this->filePath = absPath.toString();
	this->directory = absPath.parent();

	this->logDebug(this->ID() + ": Preparing DirectoryWatcher for folder '" + this->directory.path() + "'");

	this->directoryWatcher = new Poco::DirectoryWatcher(this->directory, 
			Poco::DirectoryWatcher::DW_ITEM_MODIFIED | Poco::DirectoryWatcher::DW_ITEM_ADDED | Poco::DirectoryWatcher::DW_ITEM_MOVED_TO, 
			Poco::DirectoryWatcher::DW_DEFAULT_SCAN_INTERVAL);
	this->directoryWatcher->itemModified += Poco::delegate(this, &OPDID_FileInputPort::fileChangedEvent);
	this->directoryWatcher->itemAdded += Poco::delegate(this, &OPDID_FileInputPort::fileChangedEvent);
	this->directoryWatcher->itemMovedTo += Poco::delegate(this, &OPDID_FileInputPort::fileChangedEvent);

	// can the file be loaded initially?
	Poco::File file(this->filePath);
	if (!file.exists())
		this->port->setError(VALUE_NOT_AVAILABLE);
	else {
		// expiry time specified?
		if (this->expiryMs > 0) {
			// determine whether the file is younger than the expiry time
			Poco::Timestamp now;
			// timestamp difference is in microseconds
			if ((now - file.getLastModified()) / 1000 < this->expiryMs)
				// file is not yet expired, reload
				this->needsReload = true;
			else
				this->port->setError(VALUE_NOT_AVAILABLE);
		}
		else
			this->port->setError(VALUE_NOT_AVAILABLE);
	}
}


///////////////////////////////////////////////////////////////////////////////
// AggregatorPort
///////////////////////////////////////////////////////////////////////////////

OPDID_AggregatorPort::Calculation::Calculation(std::string id) : OPDI_DialPort(id.c_str()) {
	this->algorithm = UNKNOWN;
	this->allowIncomplete = false;
}

void OPDID_AggregatorPort::Calculation::calculate(OPDID_AggregatorPort* aggregator) {
	aggregator->logExtreme(this->ID() + ": Calculating new value");
	if ((aggregator->values.size() < aggregator->totalValues) && !this->allowIncomplete)
		aggregator->logDebug(this->ID() + ": Cannot compute result because not all values have been collected and AllowIncomplete is false");
	else {
		// copy values, multiply if necessary
		std::vector<int64_t> values = aggregator->values;
		if (aggregator->multiplier != 1) {
			std::transform(values.begin(), values.end(), values.begin(), std::bind1st(std::multiplies<int64_t>(), aggregator->multiplier));
		}
		switch (this->algorithm) {
		case DELTA: {
			int64_t newValue = values.at(values.size() - 1) - values.at(0);
			aggregator->logDebug(this->ID() + ": New value according to Delta algorithm: " + this->to_string(newValue));
			if ((newValue >= this->getMin()) && (newValue <= this->getMax()))
				this->setPosition(newValue);
			else
				aggregator->logWarning(this->ID() + ": Cannot set new position: Calculated delta value is out of range: " + this->to_string(newValue));
			break;
		}
		case ARITHMETIC_MEAN: {
			int64_t sum = std::accumulate(values.begin(), values.end(), (int64_t)0);
			int64_t mean = sum / values.size();
			aggregator->logDebug(this->ID() + ": New value according to ArithmeticMean algorithm: " + this->to_string(mean));
			if ((mean >= this->getMin()) && (mean <= this->getMax()))
				this->setPosition(mean);
			else
				aggregator->logWarning(this->ID() + ": Cannot set new position: Calculated average value is out of range: " + this->to_string(mean));
			break;
		}
		case MINIMUM: {
			auto minimum = std::min_element(values.begin(), values.end());
			if (minimum == values.end())
				this->setError(VALUE_NOT_AVAILABLE);
			int64_t min = *minimum;
			aggregator->logDebug(this->ID() + ": New value according to Minimum algorithm: " + this->to_string(min));
			if ((min >= this->getMin()) && (min <= this->getMax()))
				this->setPosition(min);
			else
				aggregator->logWarning(this->ID() + ": Cannot set new position: Calculated minimum value is out of range: " + this->to_string(min));
			break;
		}
		case MAXIMUM: {
			auto maximum = std::max_element(values.begin(), values.end());
			if (maximum == values.end())
				this->setError(VALUE_NOT_AVAILABLE);
			int64_t max = *maximum;
			aggregator->logDebug(this->ID() + ": New value according to Maximum algorithm: " + this->to_string(max));
			if ((max >= this->getMin()) && (max <= this->getMax()))
				this->setPosition(max);
			else
				aggregator->logWarning(this->ID() + ": Cannot set new position: Calculated minimum value is out of range: " + this->to_string(max));
			break;
		}
		default:
			throw Poco::ApplicationException("Algorithm not supported");
		}
	}
}

// OPDID_AggregatorPort class implementation

void OPDID_AggregatorPort::persist() {
	// update persistent storage?
	if (this->isPersistent() && (this->opdid->persistentConfig != nullptr)) {
		try {
			if (this->opdid->shutdownRequested)
				this->logVerbose(this->ID() + ": Trying to persist aggregator values on shutdown");
			else
				this->logDebug(this->ID() + "Trying to persist aggregator values");
			if (this->values.size() == 0) {
				this->opdid->persistentConfig->remove(this->ID() + ".Time");
				this->opdid->persistentConfig->remove(this->ID() + ".Values");
			}
			else {
				this->opdid->persistentConfig->setUInt64(this->ID() + ".Time", this->lastQueryTime);
				std::stringstream ss;
				auto vit = this->values.cbegin();
				while (vit != this->values.cend()) {
					if (vit != this->values.cbegin())
						ss << ",";
					ss << this->to_string(*vit);
					++vit;
				}
				this->opdid->persistentConfig->setString(this->ID() + ".Values", ss.str());
			}
			this->opdid->savePersistentConfig();
		}
		catch (Poco::Exception& e) {
			this->logWarning(this->ID() + ": Error persisting aggregator values: " + e.message());
		}
		catch (std::exception& e) {
			this->logWarning(this->ID() + ": Error persisting aggregator values: " + e.what());
		}
	}
}

void OPDID_AggregatorPort::resetValues(std::string reason, AbstractOPDID::LogVerbosity logVerbosity, bool clearPersistent) {
	switch (logVerbosity) {
	case AbstractOPDID::EXTREME:
		this->logExtreme(this->ID() + ": Resetting aggregator; values are now unavailable because: " + reason); break;
	case AbstractOPDID::DEBUG:
		this->logDebug(this->ID() + ": Resetting aggregator; values are now unavailable because: " + reason); break;
	case AbstractOPDID::VERBOSE:
		this->logVerbose(this->ID() + ": Resetting aggregator; values are now unavailable because: " + reason); break;
	case AbstractOPDID::NORMAL:
		this->logNormal(this->ID() + ": Resetting aggregator; values are now unavailable because: " + reason); break;
	default: break;
	}
	// indicate errors on all calculations
	auto it = this->calculations.begin();
	while (it != this->calculations.end()) {
		(*it)->setError(OPDI_Port::VALUE_NOT_AVAILABLE);
		++it;
	}
	this->values.clear();
	// remove values from persistent storage
	if (clearPersistent && this->isPersistent() && (this->opdid->persistentConfig != nullptr)) {
		this->opdid->persistentConfig->remove(this->ID() + ".Time");
		this->opdid->persistentConfig->remove(this->ID() + ".Values");
		this->opdid->savePersistentConfig();
	}
	// clear history of associated port
	if (this->setHistory && this->historyPort != nullptr)
		this->historyPort->clearHistory();
}

uint8_t OPDID_AggregatorPort::doWork(uint8_t canSend) {
	uint8_t result = OPDI_DigitalPort::doWork(canSend);
	if (result != OPDI_STATUS_OK)
		return result;

	// disabled?
	if (this->line != 1) {
		// this->logExtreme(this->ID() + ": Aggregator is disabled");
		return OPDI_STATUS_OK;
	}

	bool valuesAvailable = false;

	if (this->firstRun) {
		this->firstRun = false;

		// try to read values from persistent storage?
		if (this->isPersistent() && (this->opdid->persistentConfig != nullptr)) {
			this->logVerbose(this->ID() + ": Trying to read persisted aggregator values");
			// read timestamp
			uint64_t persistTime = this->opdid->persistentConfig->getUInt64(this->ID() + ".Time", 0);
			// timestamp acceptable? must be in the past and within the query interval
			int64_t elapsed = opdi_get_time_ms() - persistTime;
			if ((elapsed > 0) && (elapsed < this->queryInterval * 1000)) {
				// remember persistent time as last query time
				this->lastQueryTime = persistTime;
				// read values
				std::string persistedValues = this->opdid->persistentConfig->getString(this->ID() + ".Values", "");
				// tokenize along commas
				std::stringstream ss(persistedValues);
				std::string item;
				while (std::getline(ss, item, ',')) {
					// parse item and store it
					try {
						int64_t value = Poco::NumberParser::parse64(item);
						this->values.push_back(value);
					}
					catch (Poco::Exception& e) {
						// any error causes a reset and aborts processing
						this->lastQueryTime = 0;
						this->resetValues("An error occurred deserializing persisted values: " + e.message(), AbstractOPDID::NORMAL);
						break;
					}
				}	// read values
				valuesAvailable = true;
				this->logVerbose(this->ID() + ": Total persisted aggregator values read: " + this->to_string(this->values.size()));
			}	// timestamp valid
			else
				this->logVerbose(this->ID() + ": Persisted aggregator values not found or outdated, timestamp was: " + to_string(persistTime));
		}	// persistence enabled
	}

	// time to read the next value?
	if (opdi_get_time_ms() - this->lastQueryTime > (uint64_t)this->queryInterval * 1000) {
		this->lastQueryTime = opdi_get_time_ms();

		double value;
		try {
			// get the port's value
			value = this->opdid->getPortValue(this->sourcePort);
			// reset error counter
			this->errors = 0;
		}
		catch (Poco::Exception &e) {
			this->logDebug(this->ID() + ": Error querying source port " + this->sourcePort->ID() + ": " + e.message());
			// error occurred; check whether there's a last value and an error tolerance
			if ((this->values.size() > 0) && (this->allowedErrors > 0) && (this->errors < this->allowedErrors)) {
				++errors;
				// fallback to last value
				value = (double)this->values.at(this->values.size() - 1);
				this->logDebug(this->ID() + ": Fallback to last read value, remaining allowed errors: " + this->to_string(this->allowedErrors - this->errors));
			}
			else {
				this->resetValues("Querying the source port " + this->sourcePort->ID() + " resulted in an error: " + e.message(), AbstractOPDID::VERBOSE);
				return OPDI_STATUS_OK;
			}
		}

		int64_t longValue = (int64_t)(value);

		this->logDebug(this->ID() + ": Newly aggregated value: " + this->to_string(longValue));

		// use first value without check
		if (this->values.size() > 0) {
			// vector overflow?
			if (this->values.size() >= this->totalValues) {
				this->values.erase(this->values.begin());
			}
			// compare against last element
			int64_t diff = this->values.at(this->values.size() - 1) - longValue;
			// diff may not exceed deltas
			if ((diff < this->minDelta) || (diff > this->maxDelta)) {
				this->logWarning(this->ID() + ": The new source port value of " + this->to_string(longValue) + " is outside of the specified limits (diff = " + this->to_string(diff) + ")");
				// an invalid value invalidates the whole calculation
				this->resetValues("The value was outside of the specified limits", AbstractOPDID::VERBOSE);
				return OPDI_STATUS_OK;
			}
			// value is ok
		}
		this->values.push_back(longValue);
		// persist values
		this->persist();
		valuesAvailable = true;
	}

	if (valuesAvailable) {
		if (this->setHistory && this->historyPort != nullptr)
			this->historyPort->setHistory(this->queryInterval, this->totalValues, this->values);
		// perform all calculations
		auto it = this->calculations.begin();
		while (it != this->calculations.end()) {
			(*it)->calculate(this);
			++it;
		}
	}
	return OPDI_STATUS_OK;
}

OPDID_AggregatorPort::OPDID_AggregatorPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->multiplier = 1;
	// default: allow all values (set absolute limits very high)
	this->minDelta = LLONG_MIN;
	this->maxDelta = LLONG_MAX;
	this->lastQueryTime = 0;
	this->sourcePort = nullptr;
	this->queryInterval = 0;
	this->totalValues = 0;
	this->setHistory = true;
	this->historyPort = nullptr;
	this->allowedErrors = 0;		// default setting allows no errors
	// an aggregator is an output only port
	this->setDirCaps(OPDI_PORTDIRCAP_OUTPUT);
	// an aggregator is enabled by default
	this->setLine(1);
	this->errors = 0;
	this->firstRun = true;
}

OPDID_AggregatorPort::~OPDID_AggregatorPort() {
	// destructor functionality: if the port ist persistent, try to persist values
	// ignore any errors during this process
	try {
		this->persist();
	}
	catch (...) {}
}

void OPDID_AggregatorPort::configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig) {
	this->opdid->configureDigitalPort(config, this);

	this->sourcePortID = this->opdid->getConfigString(config, "SourcePort", "", true);

	this->queryInterval = config->getInt("Interval", 0);
	if (this->queryInterval <= 0) {
		throw Poco::DataException(this->ID() + ": Please specify a positive value for Interval (in seconds): " + this->to_string(this->queryInterval));
	}

	this->totalValues = config->getInt("Values", 0);
	if (this->totalValues <= 1) {
		throw Poco::DataException(this->ID() + ": Please specify a number greater than 1 for Values: " + this->to_string(this->totalValues));
	}

	this->multiplier = config->getInt("Multiplier", this->multiplier);
	this->minDelta = config->getInt64("MinDelta", this->minDelta);
	this->maxDelta = config->getInt64("MaxDelta", this->maxDelta);
	this->allowedErrors = config->getInt("AllowedErrors", this->allowedErrors);

	this->setHistory = config->getBool("SetHistory", this->setHistory);
	this->historyPortID = this->opdid->getConfigString(config, "HistoryPort", "", false);

	// enumerate calculations
	this->logVerbose(std::string("Enumerating Aggregator calculations: ") + this->ID() + ".Calculations");

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodes = config->createView("Calculations");

	// get list of calculations
	Poco::Util::AbstractConfiguration::Keys calculations;
	nodes->keys("", calculations);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of calculations keys (by priority)
	for (auto it = calculations.begin(); it != calculations.end(); ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		auto nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			++nli;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (!this->setHistory && orderedItems.size() == 0) {
		this->logWarning(std::string("No calculations configured in node ") + this->ID() + ".Calculations and history is disabled; is this intended?");
	}

	// go through items, create calculation objects
	auto nli = orderedItems.begin();
	while (nli != orderedItems.end()) {
		std::string nodeName = nli->get<1>();
		this->logVerbose("Setting up aggregator calculation for node: " + nodeName);

		// get calculation section from the configuration
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> calculationConfig = parentConfig->createView(nodeName);

		// get type (required)
		std::string type = this->opdid->getConfigString(calculationConfig, "Type", "", true);

		// type must be "DialPort"
		if (type != "DialPort")
			throw Poco::DataException(this->ID() + ": Invalid type for calculation section, must be 'DialPort': " + nodeName);

		// creat the dial port for the calculation
		Calculation* calc = new Calculation(nodeName);
		// initialize the port default values (taken from this port)
		calc->setGroup(this->group);
		calc->setRefreshMode(this->refreshMode);

		// configure the dial port
		this->opdid->configureDialPort(calculationConfig, calc);
		// these ports are always readonly (because they contain the result of a calculation)
		calc->setReadonly(true);

		std::string algStr = calculationConfig->getString("Algorithm", "");

		if (algStr == "Delta") {
			calc->algorithm = DELTA;
			calc->allowIncomplete = false;
		} else
		if (algStr == "ArithmeticMean" || algStr == "Average") {
			calc->algorithm = ARITHMETIC_MEAN;
			calc->allowIncomplete = true;
		} else
		if (algStr == "Minimum") {
			calc->algorithm = MINIMUM;
			calc->allowIncomplete = true;
		} else
		if (algStr == "Maximum") {
			calc->algorithm = MAXIMUM;
			calc->allowIncomplete = true;
		} else
			throw Poco::DataException(this->ID() + ": Algorithm unsupported or not specified; expected 'Delta', 'ArithmeticMean'/'Average', 'Minimum', or 'Maximum': " + algStr);

		// override allowIncomplete flag if specified
		calc->allowIncomplete = calculationConfig->getBool("AllowIncomplete", calc->allowIncomplete);

		this->calculations.push_back(calc);
		
		// add port to OPDID
		this->opdid->addPort(calc);

		++nli;
	}

	// allocate vector
	this->values.reserve(this->totalValues);

	// set initial state
	this->resetValues("Setting initial state", AbstractOPDID::VERBOSE, false);
}

void OPDID_AggregatorPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find source port; throws errors if something required is missing
	this->sourcePort = this->findPort(this->getID(), "SourcePort", this->sourcePortID, true);
	this->historyPort = this->sourcePort;
	if (!this->historyPortID.empty())
		this->historyPort = this->findPort(this->getID(), "HistoryPort", this->historyPortID, true);
}

void OPDID_AggregatorPort::setLine(uint8_t newLine) {
	// if being deactivated, reset values and ports to error
	if ((this->line == 1) && (newLine == 0))
		this->resetValues("Aggregator was deactivated", AbstractOPDID::VERBOSE);
	OPDI_DigitalPort::setLine(newLine);
}

///////////////////////////////////////////////////////////////////////////////
// CounterPort
///////////////////////////////////////////////////////////////////////////////

OPDID_CounterPort::OPDID_CounterPort(AbstractOPDID *opdid, const char *id) : OPDI_DialPort(id), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	// default: increment by one
	this->increment = { 1 };
	// default: period is one second
	this->periodMs = { 1000 };
	this->lastActionTime = 0;
}

void OPDID_CounterPort::configure(Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid->configureDialPort(nodeConfig, this);

	this->periodMs.initialize(this, "Period", nodeConfig->getString("Period", this->to_string(this->periodMs.value())));
	this->increment.initialize(this, "Increment", nodeConfig->getString("Increment", this->to_string(this->increment.value())));
}

void OPDID_CounterPort::prepare() {
	// start incrementing from now
	this->lastActionTime = opdi_get_time_ms();
}

void OPDID_CounterPort::doIncrement() {
	// get current increment from value resolver
	int64_t increment = this->increment.value();
	this->lastActionTime = opdi_get_time_ms();
	this->setPosition(this->position + increment);
}

uint8_t OPDID_CounterPort::doWork(uint8_t canSend) {
	OPDI_DialPort::doWork(canSend);

	// get current period from value resolver
	int64_t period = this->periodMs.value();
	// time up? negative periods disable periodic increments
	if ((period > 0) && (opdi_get_time_ms() - this->lastActionTime > (uint64_t)period)) {
		this->doIncrement();
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// TriggerPort
///////////////////////////////////////////////////////////////////////////////

OPDID_TriggerPort::OPDID_TriggerPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	this->line = 1;	// default: active

	this->counterPort = nullptr;
}

void OPDID_TriggerPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	std::string triggerTypeStr = config->getString("Trigger", "RisingEdge");
	if (triggerTypeStr == "RisingEdge")
		this->triggerType = RISING_EDGE;
	else
	if (triggerTypeStr == "FallingEdge")
		this->triggerType = FALLING_EDGE;
	else
	if (triggerTypeStr == "Both")
		this->triggerType = BOTH;
	else
		if (triggerTypeStr != "")
			throw Poco::DataException(this->ID() + ": Invalid specification for 'Trigger', expected 'RisingEdge', 'FallingEdge', or 'Both'");

	std::string changeTypeStr = config->getString("Change", "Toggle");
	if (changeTypeStr == "SetHigh")
		this->changeType = SET_HIGH;
	else
	if (changeTypeStr == "SetLow")
		this->changeType = SET_LOW;
	else
	if (changeTypeStr == "Toggle")
		this->changeType = TOGGLE;
	else
		if (changeTypeStr != "")
			throw Poco::DataException(this->ID() + ": Invalid specification for 'Change', expected 'SetHigh', 'SetLow', or 'Toggle'");

	this->inputPortStr = config->getString("InputPorts", "");
	if (this->inputPortStr == "")
		throw Poco::DataException("Expected at least one input port for TriggerPort");

	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");
	this->counterPortStr = config->getString("CounterPort", "");
}

void OPDID_TriggerPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	// deactivated?
	if (line == 0) {
		// reset all input port states to "unknown"
		PortDataList::iterator it = this->portDataList.begin();
		while (it != this->portDataList.end()) {
			(*it).set<1>(UNKNOWN);
			++it;
		}
	}
}

void OPDID_TriggerPort::prepare() {
	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	DigitalPortList inputPorts;
	this->findDigitalPorts(this->getID(), "InputPorts", this->inputPortStr, inputPorts);
	// go through input ports, build port state list
	DigitalPortList::const_iterator it = inputPorts.begin();
	while (it != inputPorts.end()) {
		PortData pd(*it, UNKNOWN);
		this->portDataList.push_back(pd);
		++it;
	}

	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);
	if (!this->counterPortStr.empty()) {
		// find port and cast to CounterPort; if type does not match this will be nullptr
		this->counterPort = dynamic_cast<OPDID_CounterPort*>(this->findPort(this->getID(), "CounterPort", this->counterPortStr, false));
	}
}

uint8_t OPDID_TriggerPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 0)
		return OPDI_STATUS_OK;

	bool changeDetected = false;
	PortDataList::iterator it = this->portDataList.begin();
	while (it != this->portDataList.end()) {
		uint8_t mode;
		uint8_t line;
		try {
			(*it).get<0>()->getState(&mode, &line);

			if ((*it).get<1>() != UNKNOWN) {
				// state change?
				switch (this->triggerType) {
				case RISING_EDGE: if (((*it).get<1>() == LOW) && (line == 1))
									changeDetected = true; break;
				case FALLING_EDGE: if (((*it).get<1>() == HIGH) && (line == 0)) 
									changeDetected = true; break;
				case BOTH: if ((*it).get<1>() != (line == 1 ? HIGH : LOW)) 
									changeDetected = true; break;
				}
			}
			// remember current state
			(*it).set<1>(line == 1 ? HIGH : LOW);
			// do not exit the loop even if a change has been detected
			// always go through all ports
		} catch (Poco::Exception&) {
			// port has an error, set it to unknown
			(*it).set<1>(UNKNOWN);
		}
		++it;
	}

	// change detected?
	if (changeDetected) {
		this->logDebug(this->ID() + ": Detected triggering change");

		// regular output ports
		DigitalPortList::iterator it = this->outputPorts.begin();
		while (it != this->outputPorts.end()) {
			try {
				if (this->changeType == SET_HIGH) {
					(*it)->setLine(1);
				} else
				if (this->changeType == SET_LOW) {
					(*it)->setLine(0);
				} else {
					// toggle
					uint8_t mode;
					uint8_t line;
					// get current output port state
					(*it)->getState(&mode, &line);
					(*it)->setLine(line == 1 ? 0 : 1);
				}
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string("Error changing port ") + (*it)->getID() + ": " + e.message());
			}
			++it;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				if (this->changeType == SET_HIGH) {
					(*it)->setLine(0);
				} else
				if (this->changeType == SET_LOW) {
					(*it)->setLine(1);
				} else {
					// toggle
					uint8_t mode;
					uint8_t line;
					// get current output port state
					(*it)->getState(&mode, &line);
					(*it)->setLine(line == 1 ? 0 : 1);
				}
			} catch (Poco::Exception &e) {
				this->opdid->logNormal(std::string("Error changing port ") + (*it)->getID() + ": " + e.message());
			}
			++it;
		}
		// increment optional counter port
		if (this->counterPort != nullptr)
			this->counterPort->doIncrement();
	}

	return OPDI_STATUS_OK;
}



#include "opdi_port.h"
#include "opdi_platformfuncs.h"

#include "OPDID_Ports.h"

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

OPDI_DigitalPort *OPDID_PortFunctions::findDigitalPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_DigitalPort *)port;
}

void OPDID_PortFunctions::findDigitalPorts(std::string configPort, std::string setting, std::string portIDs, DigitalPortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_DigitalPort *port = this->findDigitalPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

OPDI_AnalogPort *OPDID_PortFunctions::findAnalogPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_ANALOG[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not an analog port: " + portID);

	return (OPDI_AnalogPort *)port;
}

void OPDID_PortFunctions::findAnalogPorts(std::string configPort, std::string setting, std::string portIDs, AnalogPortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_AnalogPort *port = this->findAnalogPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Logic Port
///////////////////////////////////////////////////////////////////////////////

OPDID_LogicPort::OPDID_LogicPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

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
				}
			}
		}
	} catch (...) {
		throw Poco::DataException(std::string("Syntax error for LogicPort ") + this->getID() + ", Function setting: Expected OR, AND, XOR, ATLEAST <n> or ATMOST <n>");
	}

	if (this->function == UNKNOWN)
		throw Poco::DataException("Unknown function specified for LogicPort: " + function);
	if ((this->function == ATLEAST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATLEAST of LogicPort: " + function);
	if ((this->function == ATMOST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATMOST of LogicPort: " + function);

	this->negate = config->getBool("Negate", false);

	this->inputPortStr = config->getString("InputPorts", "");
	if (this->inputPortStr == "")
		throw Poco::DataException("Expected at least one input port for LogicPort");

	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");
}

void OPDID_LogicPort::setDirCaps(const char *dirCaps) {
	throw PortError("The direction capabilities of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setMode(uint8_t mode) {
	throw PortError("The mode of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setLine(uint8_t line) {
	throw PortError("The line of a LogicPort cannot be set directly");
}

void OPDID_LogicPort::prepare() {
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
			this->opdid->log(std::string("Querying port ") + (*it)->getID() + ": " + e.message());
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
	case XOR: if (highCount == 1) 
				newLine = (this->negate ? 0 : 1);
		break;
	case ATLEAST: if (highCount >= this->funcN) 
					  newLine = (this->negate ? 0 : 1);
		break;
	case ATMOST: if (highCount <= this->funcN) 
					 newLine = (this->negate ? 0 : 1);
		break;
	}

	// change detected?
	if (newLine != this->line) {
		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			opdid->log(std::string(this->id) + ": Detected line change (" + this->to_string(highCount) + " of " + this->to_string(this->inputPorts.size()) + " inputs port are High");
	
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
					if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
						opdid->log(std::string(this->id) + ": Changing line of output port " + (*it)->getID() + " to " + (newLine == 0 ? "Low" : "High"));
					(*it)->setLine(newLine);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
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
					if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
						opdid->log(std::string(this->id) + ": Changing line of inverse output port " + (*it)->getID() + " to " + (newLine == 0 ? "High" : "Low"));
					(*it)->setLine((newLine == 0 ? 1 : 0));
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Pulse Port
///////////////////////////////////////////////////////////////////////////////

OPDID_PulsePort::OPDID_PulsePort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

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

	this->period = config->getInt("Period", -1);
	if (this->period <= 0)
		throw Poco::DataException("Specify a positive integer value for the Period setting of a PulsePort: " + this->to_string(this->period));

	this->periodPortStr = config->getString("PeriodPort", "");
	if (this->periodPortStr != "") {
		this->maxPeriod = config->getInt("MaxPeriod", -1);
		if (this->maxPeriod <= 0)
			throw Poco::DataException("A period port has been specified. You must specify a positive integer value for MaxPeriod, too: " + this->to_string(this->maxPeriod));
	}
	
	// duty cycle is specified in percent
	this->dutyCycle = config->getDouble("DutyCycle", 50) / 100.0;
	this->dutyCyclePortStr = config->getString("DutyCyclePort", "");
}

void OPDID_PulsePort::setDirCaps(const char *dirCaps) {
	throw PortError("The direction capabilities of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setMode(uint8_t mode) {
	throw PortError("The mode of a PulsePort cannot be changed");
}

void OPDID_PulsePort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "EnablePorts", this->enablePortStr, this->enablePorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);

	this->periodPort = this->findAnalogPort(this->getID(), "PeriodPort", this->periodPortStr, false);
	this->dutyCyclePort = this->findAnalogPort(this->getID(), "DutyCyclePort", this->dutyCyclePortStr, false);
}

uint8_t OPDID_PulsePort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// default: pulse is enabled if the line is High
	bool enabled = (this->line == 1);

	if (this->enablePorts.size() > 0) {
		// count how many enable ports are High
		int highCount = 0;
		DigitalPortList::iterator it = this->enablePorts.begin();
		while (it != this->enablePorts.end()) {
			uint8_t mode;
			uint8_t line;
			try {
				(*it)->getState(&mode, &line);
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Querying port ") + (*it)->getID() + ": " + e.message());
			}
			highCount += line;
			it++;
		}
		// pulse is enabled if there is at least one EnabledPort with High
		enabled |= highCount > 0;
	}

	// period port specified?
	if (this->periodPort != NULL) {
		// query period port for the current relative value
		this->period = (double)this->maxPeriod * this->periodPort->getRelativeValue();
	}

	// duty cycle port specified?
	if (this->dutyCyclePort != NULL) {
		// query duty cycle port for the current relative value
		this->dutyCycle = this->dutyCyclePort->getRelativeValue();
	}

	// determine new line level 
	uint8_t newState = this->pulseState;

	if (enabled) {
		// check whether the time for state change has been reached
		uint64_t timeDiff = opdi_get_time_ms() - this->lastStateChangeTime;
		// current state (logical) Low?
		if (this->pulseState == (this->negate ? 1 : 0)) {
			// time up to High reached?
			if (timeDiff > this->period * (1.0 - this->dutyCycle)) 
				// switch to (logical) High
				newState = (this->negate ? 0 : 1);
		} else {
			// time up to Low reached?
			if (timeDiff > this->period * this->dutyCycle)
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
		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE) {
			opdid->log(std::string(this->id) + ": Changing pulse to " + (newState == 1 ? "High" : "Low") + " (dTime: " + to_string(opdi_get_time_ms() - this->lastStateChangeTime) + " ms)");
		}
	
		this->lastStateChangeTime = opdi_get_time_ms();
		
		// set the new state
		this->pulseState = newState;

		DigitalPortList::iterator it = this->outputPorts.begin();
		// regular output ports
		while (it != this->outputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line != newState) {
					if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
						opdid->log(std::string(this->id) + ": Changing line of output port " + (*it)->getID() + " to " + (newState == 0 ? "Low" : "High"));
					(*it)->setLine(newState);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
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
				if (line == newState) {
					if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
						opdid->log(std::string(this->id) + ": Changing line of inverse output port " + (*it)->getID() + " to " + (newState == 0 ? "High" : "Low"));
					(*it)->setLine((newState == 0 ? 1 : 0));
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}


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
	this->setHidden(config->getBool("Hidden", false));

	std::string portLabel = config->getString("Label", this->id);
	this->setLabel(portLabel.c_str());

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

	std::string autoRefreshPorts = config->getString("AutoRefresh", "");
	this->setAutoRefreshPorts(autoRefreshPorts);
/*
	// SelfRefreshTime is not supported (port state does automatically cause a refresh if necessary)

	int time = config->getInt("SelfRefreshTime", -1);
	if (time >= 0) {
		this->setSelfRefreshTime(time);
	}
*/
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
	
		// will trigger AutoRefresh
		OPDI_DigitalPort::setLine(newLine);
		this->refresh();

		std::vector<OPDI_Port *> portsToRefresh;
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
					if (!(*it)->isHidden())
						// refresh port (later)
						portsToRefresh.push_back(*it);
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
					if (!(*it)->isHidden())
						// refresh port (later)
						portsToRefresh.push_back(*it);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// refresh the updated ports
		if (portsToRefresh.size() > 0) {
			portsToRefresh.push_back(NULL);
			this->opdi->refresh(&portsToRefresh[0]);
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
	// set the line to an invalid state
	// this will trigger the detection logic in the first call of doWork
	// so that all dependent output ports will be set to a defined state
	this->line = -1;
	this->counter = 0;
	this->lastStateChangeTime = 0;
}

OPDID_PulsePort::~OPDID_PulsePort() {
}

void OPDID_PulsePort::configure(Poco::Util::AbstractConfiguration *config) {
	this->setHidden(config->getBool("Hidden", false));

	std::string portLabel = config->getString("Label", this->id);
	this->setLabel(portLabel.c_str());

	this->negate = config->getBool("Negate", false);

	this->enablePortStr = config->getString("EnablePorts", "");
	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");

	this->frequency = config->getInt("Frequency", -1);
	if (this->frequency <= 0)
		throw Poco::DataException("Specify a positive integer value for the Frequency setting of a PulsePort: " + this->to_string(this->frequency));

	this->frequencyPortStr = config->getString("FrequencyPort", "");
	if (this->frequencyPortStr != "") {
		this->maxFrequency = config->getInt("MaxFrequency", -1);
		if (this->maxFrequency <= 0)
			throw Poco::DataException("A frequency port has been specified. You must specify a positive integer value for the MaxFrequency, too: " + this->to_string(this->maxFrequency));
	}
	
	// duty cycle is specified in percent
	this->dutyCycle = config->getDouble("DutyCycle", 50) / 100.0;
	this->dutyCyclePortStr = config->getString("DutyCyclePort", "");

	std::string autoRefreshPorts = config->getString("AutoRefresh", "");
	this->setAutoRefreshPorts(autoRefreshPorts);
}

void OPDID_PulsePort::setDirCaps(const char *dirCaps) {
	throw PortError("The direction capabilities of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setMode(uint8_t mode) {
	throw PortError("The mode of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setLine(uint8_t line) {
	throw PortError("The line of a PulsePort cannot be set directly");
}

void OPDID_PulsePort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "EnablePorts", this->enablePortStr, this->enablePorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);

	this->frequencyPort = this->findAnalogPort(this->getID(), "FrequencyPort", this->frequencyPortStr, false);
	this->dutyCyclePort = this->findAnalogPort(this->getID(), "DutyCyclePort", this->dutyCyclePortStr, false);
}

uint8_t OPDID_PulsePort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// default: pulse is enabled
	bool enabled = true;

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
		enabled = highCount > 0;
	}

	// frequency port specified?
	if (this->frequencyPort != NULL) {
		// query frequency port for the current relative value
		this->frequency = (double)this->maxFrequency * this->frequencyPort->getRelativeValue();
	}

	// duty cycle port specified?
	if (this->dutyCyclePort != NULL) {
		// query duty cycle port for the current relative value
		this->dutyCycle = this->dutyCyclePort->getRelativeValue();
	}

	// determine status (default depends on the negate flag)
	uint8_t newLine = (this->negate ? 1 : 0);

	if (enabled) {

		this->counter++;

		// counter above threshold?
		if (this->counter > this->frequency * this->dutyCycle)
			newLine = (this->negate ? 0 : 1);
		
		// limit reached?
		if (this->counter > this->frequency)
			counter = 0;
	}

	// evaluate function

	// change detected?
	if (newLine != this->line) {
		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE) {
			opdid->log(std::string(this->id) + ": Changing pulse to " + (newLine == 1 ? "High" : "Low"));
	
			if (this->lastStateChangeTime > 0) {
				// calculate counts per ms
				int32_t counterDelta = (this->counter > 0 ? this->frequency * this->dutyCycle : this->frequency - this->frequency * this->dutyCycle);
				if (opdi_get_time_ms() > this->lastStateChangeTime)
					opdid->log(std::string(this->id) + ": Received " + to_string(counterDelta / (opdi_get_time_ms() - this->lastStateChangeTime)) + " ticks per ms");
			}
		}
	
		this->lastStateChangeTime = opdi_get_time_ms();
		this->lastStateChangeCounter = counter;
		
		// will trigger AutoRefresh
		OPDI_DigitalPort::setLine(newLine);
		this->refresh();

		std::vector<OPDI_Port *> portsToRefresh;
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
					if (!(*it)->isHidden())
						// refresh port (later)
						portsToRefresh.push_back(*it);
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
					if (!(*it)->isHidden())
						// refresh port (later)
						portsToRefresh.push_back(*it);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// refresh the updated ports
		if (portsToRefresh.size() > 0) {
			portsToRefresh.push_back(NULL);
			this->opdi->refresh(&portsToRefresh[0]);
		}
	}

	return OPDI_STATUS_OK;
}

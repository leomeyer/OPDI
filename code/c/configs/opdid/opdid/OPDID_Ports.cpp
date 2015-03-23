
#include "opdi_port.h"

#include "OPDID_Ports.h"


OPDID_DigitalLogicPort::OPDID_DigitalLogicPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

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

OPDID_DigitalLogicPort::~OPDID_DigitalLogicPort() {
}

void OPDID_DigitalLogicPort::configure(Poco::Util::AbstractConfiguration *config) {
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
		throw Poco::DataException(std::string("Syntax error for DigitalLogicPort ") + this->getID() + ", Function setting: Expected OR, AND, XOR, ATLEAST <n> or ATMOST <n>");
	}

	if (this->function == UNKNOWN)
		throw Poco::DataException("Unknown function specified for DigitalLogic port: " + function);
	if ((this->function == ATLEAST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATLEAST of DigitalLogic port: " + function);
	if ((this->function == ATMOST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATMOST of DigitalLogic port: " + function);

	this->negate = config->getBool("Negate", false);

	this->inputPortStr = config->getString("InputPorts", "");
	if (this->inputPortStr == "")
		throw Poco::DataException("Expected at least one input port for DigitalLogic port");

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

void OPDID_DigitalLogicPort::setDirCaps(const char *dirCaps) {
	throw PortError("The direction capabilities of a DigitalLogic port cannot be changed");
}

void OPDID_DigitalLogicPort::setMode(uint8_t mode) {
	throw PortError("The mode of a DigitalLogic port cannot be changed");
}

void OPDID_DigitalLogicPort::setLine(uint8_t line) {
	throw PortError("The line of a DigitalLogic port cannot be set directly");
}

OPDI_DigitalPort *OPDID_DigitalLogicPort::findDigitalPort(std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdi->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException("Port required by Setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException("Port required by Setting " + setting + " is not a digital port: " + portID);

	return (OPDI_DigitalPort *)port;
}

void OPDID_DigitalLogicPort::findDigitalPorts(std::string setting, std::string portIDs, PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_DigitalPort *port = this->findDigitalPort(setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

void OPDID_DigitalLogicPort::prepare() {
	OPDI_DigitalPort::prepare();

	if (this->function == UNKNOWN)
		throw PortError(std::string("DigitalLogic function is unknown; port not configured correctly: ") + this->id);

	// find ports; throws errors if something required is missing
	this->findDigitalPorts("InputPorts", this->inputPortStr, this->inputPorts);
	this->findDigitalPorts("OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts("InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);
}

uint8_t OPDID_DigitalLogicPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// count how many input ports are High
	int highCount = 0;
	PortList::iterator it = this->inputPorts.begin();
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
		PortList::iterator it = this->outputPorts.begin();
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

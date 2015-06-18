
#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

OPDID_PortFunctions::OPDID_PortFunctions() {
	this->logVerbosity = AbstractOPDID::UNKNOWN;
}

OPDI_Port *OPDID_PortFunctions::findPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	return port;
}

void OPDID_PortFunctions::findPorts(std::string configPort, std::string setting, std::string portIDs, PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		if (item == "*") {
			// add all ports
			portList = this->opdid->getPorts();
		} else
		// ignore empty items
		if (item != "") {
			OPDI_Port *port = this->findPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

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

OPDI_SelectPort *OPDID_PortFunctions::findSelectPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_SelectPort *)port;
}

void OPDID_PortFunctions::configureVerbosity(Poco::Util::AbstractConfiguration* config) {
	std::string logVerbosityStr = config->getString("LogVerbosity", "");

	// set log verbosity only if it's not already set
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) && (logVerbosityStr != "")) {
		if (logVerbosityStr == "Quiet") {
			this->logVerbosity = AbstractOPDID::QUIET;
		} else
		if (logVerbosityStr == "Normal") {
			this->logVerbosity = AbstractOPDID::NORMAL;
		} else
		if (logVerbosityStr == "Verbose") {
			this->logVerbosity = AbstractOPDID::VERBOSE;
		} else
		if (logVerbosityStr == "Debug") {
			this->logVerbosity = AbstractOPDID::DEBUG;
		} else
		if (logVerbosityStr == "Extreme") {
			this->logVerbosity = AbstractOPDID::EXTREME;
		} else
			throw Poco::InvalidArgumentException("Verbosity level unknown (expected one of 'Quiet', 'Normal', 'Verbose', 'Debug' or 'Extreme')", logVerbosityStr);
	}
}

void OPDID_PortFunctions::logWarning(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity > AbstractOPDID::QUIET)) {
		this->opdid->logWarning(message);
	}
}

void OPDID_PortFunctions::logNormal(std::string message) {
	if (((this->logVerbosity == AbstractOPDID::UNKNOWN) && (this->opdid->logVerbosity >= AbstractOPDID::NORMAL)) || (this->logVerbosity >= AbstractOPDID::NORMAL)) {
		this->opdid->log(message);
	}
}

void OPDID_PortFunctions::logVerbose(std::string message) {
	if (((this->logVerbosity == AbstractOPDID::UNKNOWN) && (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)) || (this->logVerbosity >= AbstractOPDID::VERBOSE)) {
		this->opdid->log(message);
	}
}

void OPDID_PortFunctions::logDebug(std::string message) {
	if (((this->logVerbosity == AbstractOPDID::UNKNOWN) && (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)) || (this->logVerbosity >= AbstractOPDID::DEBUG)) {
		this->opdid->log(message);
	}
}

void OPDID_PortFunctions::logExtreme(std::string message) {
	if (((this->logVerbosity == AbstractOPDID::UNKNOWN) && (this->opdid->logVerbosity >= AbstractOPDID::EXTREME)) || (this->logVerbosity >= AbstractOPDID::EXTREME)) {
		this->opdid->log(message);
	}
}



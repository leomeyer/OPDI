
#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

OPDID_PortFunctions::OPDID_PortFunctions(std::string id) {
	this->logVerbosity = AbstractOPDID::UNKNOWN;
	this->portFunctionID = id;
	this->opdid = nullptr;
}

OPDI_Port* OPDID_PortFunctions::findPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	OPDI_Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	return port;
}

void OPDID_PortFunctions::findPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, OPDI::PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		if (item == "*") {
			// add all ports
			portList = this->opdid->getPorts();
		} else
		// ignore empty items
		if (!item.empty()) {
			OPDI_Port* port = this->findPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

OPDI_DigitalPort* OPDID_PortFunctions::findDigitalPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	OPDI_Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_DigitalPort*)port;
}

void OPDID_PortFunctions::findDigitalPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, DigitalPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			OPDI_DigitalPort* port = this->findDigitalPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

OPDI_AnalogPort* OPDID_PortFunctions::findAnalogPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	OPDI_Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_ANALOG[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not an analog port: " + portID);

	return (OPDI_AnalogPort*)port;
}

void OPDID_PortFunctions::findAnalogPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, AnalogPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			OPDI_AnalogPort* port = this->findAnalogPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

OPDI_SelectPort* OPDID_PortFunctions::findSelectPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	OPDI_Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_SelectPort*)port;
}

void OPDID_PortFunctions::logWarning(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity > AbstractOPDID::QUIET)) {
		this->opdid->logWarning(message);
	}
}

void OPDID_PortFunctions::logNormal(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::NORMAL)) {
		this->opdid->logNormal(message);
	}
}

void OPDID_PortFunctions::logVerbose(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::VERBOSE)) {
		this->opdid->logVerbose(message);
	}
}

void OPDID_PortFunctions::logDebug(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::DEBUG)) {
		this->opdid->logDebug(message);
	}
}

void OPDID_PortFunctions::logExtreme(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::EXTREME)) {
		this->opdid->logExtreme(message);
	}
}

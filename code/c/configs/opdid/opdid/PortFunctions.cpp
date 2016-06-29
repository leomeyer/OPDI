#include "PortFunctions.h"

namespace opdid {

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

PortFunctions::PortFunctions(std::string id) {
	this->logVerbosity = AbstractOPDID::UNKNOWN;
	this->portFunctionID = id;
	this->opdid = nullptr;
}

opdi::Port* PortFunctions::findPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	return port;
}

void PortFunctions::findPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, PortList &portList) {
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
			opdi::Port* port = this->findPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

opdi::DigitalPort* PortFunctions::findDigitalPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "digital"
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (opdi::DigitalPort*)port;
}

void PortFunctions::findDigitalPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, DigitalPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			opdi::DigitalPort* port = this->findDigitalPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

opdi::AnalogPort* PortFunctions::findAnalogPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "analog"
	if (port->getType()[0] != OPDI_PORTTYPE_ANALOG[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not an analog port: " + portID);

	return (opdi::AnalogPort*)port;
}

void PortFunctions::findAnalogPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, AnalogPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			opdi::AnalogPort* port = this->findAnalogPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

opdi::SelectPort* PortFunctions::findSelectPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "select"
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a select port: " + portID);

	return (opdi::SelectPort*)port;
}

void PortFunctions::logWarning(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity > AbstractOPDID::QUIET)) {
		this->opdid->logWarning(message);
	}
}

void PortFunctions::logNormal(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::NORMAL)) {
		this->opdid->logNormal(message, this->logVerbosity);
	}
}

void PortFunctions::logVerbose(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::VERBOSE)) {
		this->opdid->logVerbose(message, this->logVerbosity);
	}
}

void PortFunctions::logDebug(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::DEBUG)) {
		this->opdid->logDebug(message, this->logVerbosity);
	}
}

void PortFunctions::logExtreme(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::EXTREME)) {
		this->opdid->logExtreme(message, this->logVerbosity);
	}
}

}		// namespace opdid
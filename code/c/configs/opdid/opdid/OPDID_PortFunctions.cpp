
#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

OPDID_PortFunctions::OPDID_PortFunctions(std::string id) {
	this->logVerbosity = AbstractOPDID::UNKNOWN;
	this->portFunctionID = id;
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

void OPDID_PortFunctions::findPorts(std::string configPort, std::string setting, std::string portIDs, OPDI::PortList &portList) {
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

void OPDID_PortFunctions::logWarning(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity > AbstractOPDID::QUIET)) {
		this->opdid->logWarning(message);
	}
}

void OPDID_PortFunctions::logNormal(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::NORMAL)) {
		this->opdid->logNormal(message);
	}
}

void OPDID_PortFunctions::logVerbose(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::VERBOSE)) {
		this->opdid->logVerbose(message);
	}
}

void OPDID_PortFunctions::logDebug(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::DEBUG)) {
		this->opdid->logDebug(message);
	}
}

void OPDID_PortFunctions::logExtreme(std::string message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::EXTREME)) {
		this->opdid->logExtreme(message);
	}
}


ValueResolver::ValueResolver() {
	this->isFixed = false;
}

void ValueResolver::initialize(OPDID_PortFunctions *origin, std::string paramName, std::string value, bool allowErrorDefault) {
	this->origin = origin;
	this->useScaleValue = false;
	this->useErrorDefault = false;
	this->port = NULL;

	// try to convert the value to a double
	if (Poco::NumberParser::tryParseFloat(value, this->fixedValue)) {
		this->isFixed = true;
	} else {
		this->isFixed = false;
		Poco::RegularExpression::MatchVec matches;
		std::string portName;
		std::string scaleStr;
		std::string defaultStr;
		// try to match full syntax
		Poco::RegularExpression reFull("(.*)\\((.*)\\)\\/(.*)");
		if (reFull.match(value, 0, matches) == 4) {
			if (!allowErrorDefault)
				throw new Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Specifying an error default value is not allowed: " + value);
			portName = value.substr(matches[1].offset, matches[1].length);
			scaleStr = value.substr(matches[2].offset, matches[2].length);
			defaultStr = value.substr(matches[3].offset, matches[3].length);
		} else {
			// try to match default value syntax
			Poco::RegularExpression reDefault("(.*)\\/(.*)");
			if (reDefault.match(value, 0, matches) == 3) {
				if (!allowErrorDefault)
					throw new Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Specifying an error default value is not allowed: " + value);
				portName = value.substr(matches[1].offset, matches[1].length);
				defaultStr = value.substr(matches[2].offset, matches[2].length);
			} else {
				// try to match scale value syntax
				Poco::RegularExpression reDefault("(.*)\\((.*)\\)");
				if (reDefault.match(value, 0, matches) == 3) {
					portName = value.substr(matches[1].offset, matches[1].length);
					scaleStr = value.substr(matches[2].offset, matches[2].length);
				} else {
					// could not match a pattern - use value as port name
					portName = value;
				}
			}
		}
		// parse values if specified
		if (scaleStr != "") {
			if (Poco::NumberParser::tryParseFloat(scaleStr, this->scaleValue)) {
				this->useScaleValue = true;
			} else
				throw new Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Invalid scale value specified; must be numeric: " + scaleStr);
		}
		if (defaultStr != "") {
			if (Poco::NumberParser::tryParseFloat(defaultStr, this->errorDefault)) {
				this->useErrorDefault = true;
			} else
				throw new Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Invalid error default value specified; must be numeric: " + defaultStr);
		}

		this->portID = portName;
	}
};

bool ValueResolver::validate(double min, double max) {
	// no fixed value? assume it's valid
	if (!this->isFixed)
		return true;

	return ((this->fixedValue >= min) && (this->fixedValue <= max));
}

bool ValueResolver::validateAsInt(int min, int max) {
	// no fixed value? assume it's valid
	if (!this->isFixed)
		return true;
	int iValue = static_cast<int>(this->fixedValue);
	return ((iValue >= min) && (iValue <= max));
}

ValueResolver::operator double() {
	if (isFixed)
		return fixedValue;
	else {
		// port not yet resolved?
		if (this->port == NULL) {
			if (this->portID == "")
				throw new Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": ValueResolver not initialized (programming error)");
			// try to resolve the port
			this->port = this->origin->findPort(this->origin->portFunctionID, this->paramName, this->portID, true);
		}
		// resolve port value to a double
		double result = 0;

		if (this->useErrorDefault) {
			try {
				result = origin->opdid->getPortValue(this->port);
			} catch (Poco::Exception &pe) {
				origin->logExtreme(this->origin->portFunctionID + ": Unable to get the value of the port " + port->ID() + ": " + pe.message());
				return this->errorDefault;
			}
		} else {
			// propagate exceptions directly
			result = origin->opdid->getPortValue(this->port);
		}

		// scale?
		if (this->useScaleValue) {
			result *= this->scaleValue;
		}

		return result;
	}
}


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

// find function delegates

opdi::Port * PortFunctions::findPort(const std::string & configPort, const std::string & setting, const std::string & portID, bool required)
{
	return this->opdid->findPort(configPort, setting, portID, required);
}

void PortFunctions::findPorts(const std::string & configPort, const std::string & setting, const std::string & portIDs, opdi::PortList & portList)
{
	this->opdid->findPorts(configPort, setting, portIDs, portList);
}

opdi::DigitalPort * PortFunctions::findDigitalPort(const std::string & configPort, const std::string & setting, const std::string & portID, bool required)
{
	return this->opdid->findDigitalPort(configPort, setting, portID, required);
}

void PortFunctions::findDigitalPorts(const std::string & configPort, const std::string & setting, const std::string & portIDs, opdi::DigitalPortList & portList)
{
	this->opdid->findDigitalPorts(configPort, setting, portIDs, portList);
}

opdi::AnalogPort * PortFunctions::findAnalogPort(const std::string & configPort, const std::string & setting, const std::string & portID, bool required)
{
	return this->opdid->findAnalogPort(configPort, setting, portID, required);
}

void PortFunctions::findAnalogPorts(const std::string & configPort, const std::string & setting, const std::string & portIDs, opdi::AnalogPortList & portList)
{
	this->opdid->findAnalogPorts(configPort, setting, portIDs, portList);
}

opdi::SelectPort * PortFunctions::findSelectPort(const std::string & configPort, const std::string & setting, const std::string & portID, bool required)
{
	return this->opdid->findSelectPort(configPort, setting, portID, required);
}

void PortFunctions::logWarning(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity > AbstractOPDID::QUIET)) {
		this->opdid->logWarning(this->portFunctionID + ": " + message);
	}
}

void PortFunctions::logNormal(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::NORMAL)) {
		this->opdid->logNormal(this->portFunctionID + ": " + message, this->logVerbosity);
	}
}

void PortFunctions::logVerbose(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::VERBOSE)) {
		this->opdid->logVerbose(this->portFunctionID + ": " + message, this->logVerbosity);
	}
}

void PortFunctions::logDebug(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::DEBUG)) {
		this->opdid->logDebug(this->portFunctionID + ": " + message, this->logVerbosity);
	}
}

void PortFunctions::logExtreme(const std::string& message) {
	if ((this->logVerbosity == AbstractOPDID::UNKNOWN) || (this->logVerbosity >= AbstractOPDID::EXTREME)) {
		this->opdid->logExtreme(this->portFunctionID + ": " + message, this->logVerbosity);
	}
}

}		// namespace opdid
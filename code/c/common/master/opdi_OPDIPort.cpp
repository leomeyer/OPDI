
#include <vector>

#include "opdi_OPDIPort.h"
#include "opdi_AbstractProtocol.h"
#include "opdi_IBasicProtocol.h"

OPDIPort::OPDIPort(IBasicProtocol& protocol, std::string id, std::string name, PortType type, PortDirCaps dirCaps) : protocol(protocol) {
	this->id = id;
	this->name = name;
	this->type = type;
	this->dirCaps = dirCaps;
}

IBasicProtocol& OPDIPort::getProtocol() {
	return protocol;
}

std::string OPDIPort::getID() {
	return id;
}

std::string OPDIPort::getName() {
	return name;
}
		
PortType OPDIPort::getType() {
	return type;
}

PortDirCaps OPDIPort::getDirCaps() {
	return dirCaps;
}

std::string OPDIPort::getDirCapsText()
{
	switch (dirCaps)
	{
	case PORTDIRCAP_UNKNOWN: return "Unknown";
	case PORTDIRCAP_INPUT: return "Input";	
	case PORTDIRCAP_OUTPUT: return "Output";
	case PORTDIRCAP_BIDIRECTIONAL: return "Bidirectional";
	}
	return "Invalid dirCaps: " + dirCaps;
}

void OPDIPort::setProtocol(IBasicProtocol& protocol) {
	this->protocol = protocol;
}

void OPDIPort::setID(std::string id) {
	this->id = id;
}

void OPDIPort::setName(std::string name) {
	this->name = name;
}

void OPDIPort::setType(PortType type) {
	this->type = type;
}

void OPDIPort::setDirCaps(PortDirCaps dir) {
	dirCaps = dir;
}
	
void OPDIPort::checkSerialForm(std::vector<std::string> parts, unsigned int count, std::string magic) 
{
	if (parts.size() != count) 
		throw ProtocolException("Serial form invalid");
	if (parts[0] != magic)
		throw ProtocolException("Serial form invalid: wrong magic");		
}

void OPDIPort::setViewAdapter(void *viewAdapter) {
	this->viewAdapter = viewAdapter;
}

void *OPDIPort::getViewAdapter() {
	return viewAdapter;
}

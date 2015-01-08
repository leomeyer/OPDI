
#include <vector>

#include "opdi_Port.h"
#include "opdi_AbstractProtocol.h"
#include "opdi_IBasicProtocol.h"

Port::Port(IBasicProtocol& protocol, std::string id, std::string name, PortType type, PortDirCaps dirCaps) : protocol(protocol) {
	this->id = id;
	this->name = name;
	this->type = type;
	this->dirCaps = dirCaps;
}

IBasicProtocol& Port::getProtocol() {
	return protocol;
}

std::string Port::getID() {
	return id;
}

std::string Port::getName() {
	return name;
}
		
PortType Port::getType() {
	return type;
}

PortDirCaps Port::getDirCaps() {
	return dirCaps;
}

std::string Port::getDirCapsText()
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

void Port::setProtocol(IBasicProtocol& protocol) {
	this->protocol = protocol;
}

void Port::setID(std::string id) {
	this->id = id;
}

void Port::setName(std::string name) {
	this->name = name;
}

void Port::setType(PortType type) {
	this->type = type;
}

void Port::setDirCaps(PortDirCaps dir) {
	dirCaps = dir;
}
	
void Port::checkSerialForm(std::vector<std::string> parts, unsigned int count, std::string magic) 
{
	if (parts.size() != count) 
		throw ProtocolException("Serial form invalid");
	if (parts[0] != magic)
		throw ProtocolException("Serial form invalid: wrong magic");		
}

void Port::setViewAdapter(void *viewAdapter) {
	this->viewAdapter = viewAdapter;
}

void *Port::getViewAdapter() {
	return viewAdapter;
}

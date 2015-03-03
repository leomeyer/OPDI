#include "opdi_AbstractProtocol.h"
#include "opdi_IBasicProtocol.h"
#include "opdi_StringTools.h"

#include "opdi_BasicDeviceCapabilities.h"

BasicDeviceCapabilities::BasicDeviceCapabilities(IBasicProtocol* protocol, int channel, std::string serialForm)
{
	int PORTS_PART = 1;
	unsigned int PART_COUNT = 2;
		
	// decode the serial representation
	std::vector<std::string> parts;
	StringTools::split(serialForm, ':', parts);
	if (parts.size() != PART_COUNT) 
		throw ProtocolException("BasicDeviceCapabilities message invalid");
	if (parts[0] != OPDI_BASIC_DEVICE_CAPABILITIES_MAGIC)
		throw ProtocolException("BasicDeviceCapabilities message invalid: incorrect magic: " + parts[0]);
	// split port IDs by comma
	std::vector<std::string> portIDs;
	StringTools::split(parts[PORTS_PART], ',', portIDs);
	// get port info for each port
	for (std::vector<std::string>::iterator iter = portIDs.begin(); iter != portIDs.end(); iter++) {
		if ((*iter) != "") {
			OPDIPort* port = protocol->getPortInfo((*iter), channel);
			// ignore NULLs
			if (port)
				ports.push_back(port);
		}
	}
}

OPDIPort* BasicDeviceCapabilities::findPortByID(std::string portID) {
	for (std::vector<OPDIPort*>::iterator iter = ports.begin(); iter != ports.end(); iter++) {
		if ((*iter)->getID() == portID)
			return (*iter);
	}
	return NULL;
}

std::vector<OPDIPort*> & BasicDeviceCapabilities::getPorts()
{
	return ports;
}

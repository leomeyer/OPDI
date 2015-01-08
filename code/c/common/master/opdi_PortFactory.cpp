#include <string>
#include <vector>

#include "opdi_IBasicProtocol.h"
#include "opdi_PortFactory.h"


Port* PortFactory::createPort(IBasicProtocol& protocol, std::vector<std::string> parts)
{
	// the "magic" in the first part decides about the port type		
	if (parts[0] == OPDI_digitalPort)
		return new DigitalPort(protocol, parts);
/*
	else if (parts[0].equals(AnalogPort.MAGIC))
		return new AnalogPort(protocol, parts);
	else if (parts[0].equals(SelectPort.MAGIC))
		return new SelectPort(protocol, parts);
	else if (parts[0].equals(DialPort.MAGIC))
		return new DialPort(protocol, parts);
	else if (parts[0].equals(StreamingPort.MAGIC))
		return new StreamingPort(protocol, parts);
*/
	else
		return NULL;

//	else
	//	throw Poco::InvalidArgumentException("Programmer error: Unknown port magic: '" + parts[0] + "'. Protocol must check supported ports");
}

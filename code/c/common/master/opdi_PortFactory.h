#ifndef __PORTFACTORY_H
#define __PORTFACTORY_H

#include "opdi_protocol_constants.h"

#include "opdi_OPDIPort.h"

/** This class serves as a factory for basic port objects.
 * 
 * @author Leo
 *
 */
class PortFactory {
	
public:
	static OPDIPort* createPort(IBasicProtocol& protocol, std::vector<std::string> parts);

};

#endif

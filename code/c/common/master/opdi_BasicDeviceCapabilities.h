
#ifndef __OPDI_BASICDEVICECAPABILITIES_H
#define __OPDI_BASICDEVICECAPABILITIES_H

#include "opdi_Port.h"

#define OPDI_BASIC_DEVICE_CAPABILITIES_MAGIC "BDC"

/** This class describes the capabilities of an OPDI device. It contains convenience methods to access ports.
 * 
 * @author Leo
 *
 */
class BasicDeviceCapabilities {

protected:
	std::vector<Port*> ports;
	
public:
	BasicDeviceCapabilities(IBasicProtocol* protocol, int channel, std::string serialForm);
	
	Port* findPortByID(std::string portID);

	std::vector<Port*> & getPorts();
};

#endif

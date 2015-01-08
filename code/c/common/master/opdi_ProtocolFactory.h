#ifndef __OPDI_PROTOCOLFACTORY_H
#define __OPDI_PROTOCOLFACTORY_H

#include "opdi_IBasicProtocol.h"
#include "opdi_IDevice.h"

/** This class is a protocol factory that selects the proper protocol for a device.
 * A protocol class must provide a constructor with parameters (IDevice device, IConnectionListener listener).
 * 
 * @author Leo
 *
 */
class ProtocolFactory {

public:
	static IBasicProtocol* getProtocol(IDevice* device, std::string magic);

};

#endif

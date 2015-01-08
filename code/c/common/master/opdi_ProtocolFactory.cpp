#include "opdi_IBasicProtocol.h"
#include "opdi_ProtocolFactory.h"
#include "opdi_BasicProtocol.h"

IBasicProtocol* ProtocolFactory::getProtocol(IDevice* device, std::string magic) {
	if (magic == "BP") {
		return new BasicProtocol(device);
	}

	return NULL;
}

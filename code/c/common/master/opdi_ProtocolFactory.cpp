#include <master\opdi_IBasicProtocol.h>
#include <master\opdi_ProtocolFactory.h>
#include <master\opdi_BasicProtocol.h>

IBasicProtocol* ProtocolFactory::getProtocol(IDevice* device, std::string magic) {
	if (magic == "BP") {
		return new BasicProtocol(device);
	}

	return NULL;
}

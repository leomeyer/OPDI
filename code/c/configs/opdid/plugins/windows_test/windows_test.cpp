
// Test plugin for OPDID (Windows version)

#include "opdi_constants.h"

#include "WindowsOPDID.h"

class DigitalTestPort : public OPDI_EmulatedDigitalPort {
public:
	DigitalTestPort();
	virtual uint8_t setLine(uint8_t line) override;
};

DigitalTestPort::DigitalTestPort() : OPDI_EmulatedDigitalPort("PluginPort", "Windows Test Plugin Port", OPDI_PORTDIRCAP_OUTPUT, 0) {}

uint8_t DigitalTestPort::setLine(uint8_t line) {
	OPDI_EmulatedDigitalPort::setLine(line);

	AbstractOPDID *opdid = (AbstractOPDID *)this->opdi;
	if (line == 0) {
		opdid->println("DigitalTestPort line set to Low");
	} else {
		opdid->println("DigitalTestPort line set to High");
	}

	return OPDI_STATUS_OK;
}

class WindowsTestOPDIDPlugin : public IOPDIDPlugin {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node);
};


void WindowsTestOPDIDPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node) {
	this->opdid = abstractOPDID;

	// add emulated test port
	DigitalTestPort *emuPort = new DigitalTestPort();
	abstractOPDID->addPort(emuPort);

	this->opdid->println("WindowsTestOPDIDPlugin setup completed successfully as node " + node);

	this->opdid->reconfigure();
}

// plugin instance factory function
extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance() {
	// return a new instance of this plugin
	return new WindowsTestOPDIDPlugin();
}
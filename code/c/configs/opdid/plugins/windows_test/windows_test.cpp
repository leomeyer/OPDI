
// Test plugin for OPDID (Windows version)

#include "opdi_constants.h"

#include "WindowsOPDID.h"

class DigitalTestPort : public OPDI_DigitalPort {
public:
	DigitalTestPort();
	virtual void setLine(uint8_t line, ChangeSource /*changeSource*/) override;
};

DigitalTestPort::DigitalTestPort() : OPDI_DigitalPort("PluginPort", "Windows Test Plugin Port", OPDI_PORTDIRCAP_OUTPUT, 0) {}

void DigitalTestPort::setLine(uint8_t line, ChangeSource /*changeSource*/) {
	OPDI_DigitalPort::setLine(line);

	AbstractOPDID *opdid = (AbstractOPDID *)this->opdi;
	if (line == 0) {
		opdid->logNormal("DigitalTestPort line set to Low");
	} else {
		opdid->logNormal("DigitalTestPort line set to High");
	}
}

class WindowsTestOPDIDPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration *config) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};

void WindowsTestOPDIDPlugin::setupPlugin(AbstractOPDID *abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = config->createView(node);

	// get port type
	std::string portType = nodeConfig->getString("Type", "");

	if (portType == "DigitalPort") {
		// add emulated test port
		DigitalTestPort *port = new DigitalTestPort();
		abstractOPDID->configureDigitalPort(nodeConfig, port);
		abstractOPDID->addPort(port);
	} else
		throw Poco::DataException("This plugin supports only node type 'DigitalPort'", portType);

	this->opdid->addConnectionListener(this);

	this->opdid->logVerbose("WindowsTestOPDIDPlugin setup completed successfully as node " + node);
}

void WindowsTestOPDIDPlugin::masterConnected() {
	this->opdid->logNormal("Test plugin: master connected");
}

void WindowsTestOPDIDPlugin::masterDisconnected() {
	this->opdid->logNormal("Test plugin: master disconnected");
}

// plugin instance factory function
extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > OPDID_MAJOR_VERSION) || (minorVersion > OPDID_MINOR_VERSION))
		throw Poco::Exception("This plugin supports only OPDID versions up to "
			OPDI_QUOTE(OPDID_MAJOR_VERSION) "." OPDI_QUOTE(OPDID_MINOR_VERSION));

	// return a new instance of this plugin
	return new WindowsTestOPDIDPlugin();
}
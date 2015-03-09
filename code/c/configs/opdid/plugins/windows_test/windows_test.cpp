
// Test plugin for OPDID (Windows version)

#include "opdi_constants.h"

#include "WindowsOPDID.h"

class DigitalTestPort : public OPDI_DigitalPort {
public:
	DigitalTestPort();
	virtual uint8_t setLine(uint8_t line) override;
};

DigitalTestPort::DigitalTestPort() : OPDI_DigitalPort("PluginPort", "Windows Test Plugin Port", OPDI_PORTDIRCAP_OUTPUT, 0) {}

uint8_t DigitalTestPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	AbstractOPDID *opdid = (AbstractOPDID *)this->opdi;
	if (line == 0) {
		opdid->log("DigitalTestPort line set to Low");
	} else {
		opdid->log("DigitalTestPort line set to High");
	}

	return OPDI_STATUS_OK;
}

class WindowsTestOPDIDPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};

void WindowsTestOPDIDPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;

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

	if (this->opdid->logVerbosity == AbstractOPDID::VERBOSE)
		this->opdid->log("WindowsTestOPDIDPlugin setup completed successfully as node " + node);
}

void WindowsTestOPDIDPlugin::masterConnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->log("Test plugin: master connected");
}

void WindowsTestOPDIDPlugin::masterDisconnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->log("Test plugin: master disconnected");
}

// plugin instance factory function
extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the WindowsTestOPDIPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new WindowsTestOPDIDPlugin();
}
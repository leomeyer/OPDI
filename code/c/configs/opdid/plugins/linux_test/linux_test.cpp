
// Test plugin for OPDID (Linux version)

#include "opdi_constants.h"

#include "LinuxOPDID.h"

class DigitalTestPort : public OPDI_DigitalPort {
public:
	DigitalTestPort();
	virtual void setLine(uint8_t line) override;
};

DigitalTestPort::DigitalTestPort() : OPDI_DigitalPort("PluginPort", "Linux Test Plugin Port", OPDI_PORTDIRCAP_OUTPUT, 0) {}

void DigitalTestPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	AbstractOPDID *opdid = (AbstractOPDID *)this->opdi;
	if (line == 0) {
		opdid->logNormal("DigitalTestPort line set to Low");
	} else {
		opdid->logNormal("DigitalTestPort line set to High");
	}
}

class LinuxTestOPDIDPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration *config);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};


void LinuxTestOPDIDPlugin::setupPlugin(AbstractOPDID *abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;

	Poco::Util::AbstractConfiguration *nodeConfig = config->createView(node);

	// get port type
	std::string portType = nodeConfig->getString("Type", "");

	if (portType == "DigitalPort") {
		// add emulated test port
		DigitalTestPort *port = new DigitalTestPort();
		abstractOPDID->configureDigitalPort(nodeConfig, port);
		abstractOPDID->addPort(port);
	} else
		throw Poco::DataException("This plugin supports only node type 'DigitalPort'", portType);

	if (this->opdid->logVerbosity == AbstractOPDID::VERBOSE)
		this->opdid->logVerbose("LinuxTestOPDIDPlugin setup completed successfully as node " + node);
}

void LinuxTestOPDIDPlugin::masterConnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->logNormal("Test plugin: master connected");
}

void LinuxTestOPDIDPlugin::masterDisconnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->logNormal("Test plugin: master disconnected");
}

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the LinuxTestOPDIPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new LinuxTestOPDIDPlugin();
}

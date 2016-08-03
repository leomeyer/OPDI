
// Test plugin for OPDID (Linux version)

#include "opdi_constants.h"

#include "LinuxOPDID.h"

namespace {

class DigitalTestPort : public opdi::DigitalPort {
public:
	DigitalTestPort();
	virtual void setLine(uint8_t line, ChangeSource changeSource = ChangeSource::CHANGESOURCE_INT) override;
};

}

DigitalTestPort::DigitalTestPort() : opdi::DigitalPort("PluginPort", "Linux Test Plugin Port", OPDI_PORTDIRCAP_OUTPUT, 0) {}

void DigitalTestPort::setLine(uint8_t line, ChangeSource /*changeSource*/) {
	opdi::DigitalPort::setLine(line);

	opdid::AbstractOPDID* opdid = (opdid::AbstractOPDID*)this->opdi;
	if (line == 0) {
		opdid->logNormal("DigitalTestPort line set to Low");
	} else {
		opdid->logNormal("DigitalTestPort line set to High");
	}
}

class LinuxTestOPDIDPlugin : public IOPDIDPlugin, public opdid::IOPDIDConnectionListener {

protected:
	opdid::AbstractOPDID* opdid;

public:
	virtual void setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* config);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};


void LinuxTestOPDIDPlugin::setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* config) {
	this->opdid = abstractOPDID;

	Poco::Util::AbstractConfiguration* nodeConfig = config->createView(node);

	// get port type
	std::string portType = nodeConfig->getString("Type", "");

	if (portType == "DigitalPort") {
		// add emulated test port
		DigitalTestPort* port = new DigitalTestPort();
		abstractOPDID->configureDigitalPort(nodeConfig, port);
		abstractOPDID->addPort(port);
	} else
		throw Poco::DataException("This plugin supports only node type 'DigitalPort'", portType);

	this->opdid->logVerbose("LinuxTestOPDIDPlugin setup completed successfully as node " + node);
}

void LinuxTestOPDIDPlugin::masterConnected() {
	this->opdid->logNormal("Test plugin: master connected");
}

void LinuxTestOPDIDPlugin::masterDisconnected() {
	this->opdid->logNormal("Test plugin: master disconnected");
}

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the LinuxTestOPDIPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new LinuxTestOPDIDPlugin();
}

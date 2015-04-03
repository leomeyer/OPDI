#include <iostream>
#include <sstream>

#include "Poco/Tuple.h"
#include "Poco/Runnable.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/DigestStream.h"
#include "Poco/MD5Engine.h"
#include "Poco/UTF16Encoding.h"
#include "Poco/UnicodeConverter.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/AutoPtr.h"
#include "Poco/NumberParser.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#ifdef _WINDOWS
#include "WindowsOPDID.h"
#endif

#ifdef linux
#include "LinuxOPDID.h"
#endif

////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WeatherPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener, public Poco::Runnable {

protected:
	std::string nodeID;

	std::string url;
	int timeoutSeconds;

	// weather data provider; at the moment only "weewx" is supported
	std::string provider;

	int refreshTimeMs;

	std::string sid;

	Poco::Thread workThread;

public:
	AbstractOPDID *opdid;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

	virtual void run(void);

	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;
};


void WeatherPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;
	this->nodeID = node;
	this->timeoutSeconds = 2;			// short timeout (assume local network)

	this->url = abstractOPDID->getConfigString(nodeConfig, "Url", "", true);
	this->timeoutSeconds = nodeConfig->getInt("Timeout", this->timeoutSeconds);

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");
	this->refreshTimeMs = nodeConfig->getInt("RefreshTime", 10000);
	if (this->refreshTimeMs < 0)
		throw Poco::DataException(node + "Please specify a non-negative RefreshTime in milliseconds");

	// enumerate keys of the plugin's nodes (in specified order)
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("Enumerating Weather nodes: " + node + ".Nodes");

	Poco::Util::AbstractConfiguration *nodes = nodeConfig->createView("Nodes");

	// get ordered list of ports
	Poco::Util::AbstractConfiguration::Keys portKeys;
	nodes->keys("", portKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of port keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = portKeys.begin(); it != portKeys.end(); ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			nli++;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		this->opdid->log("Warning: No ports configured in node " + node + ".Nodes; is this intended?");
	}

	// go through items, create ports in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {

		std::string nodeName = nli->get<1>();

		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log("Setting up FritzBox port(s) for node: " + nodeName);

		// get port section from the configuration
		Poco::Util::AbstractConfiguration *portConfig = abstractOPDID->getConfiguration()->createView(nodeName);

		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, "Type", "", true);

		if (portType == "FritzDECT200") {

		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		nli++;
	}

	this->opdid->addConnectionListener(this);

	this->workThread.setName(node + " work thread");
	this->workThread.start(*this);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": FritzBoxPlugin setup completed successfully");
}

void WeatherPlugin::masterConnected() {
	// when the master connects, login to the FritzBox
	//this->queue.enqueueNotification(new ActionNotification(ActionNotification::LOGIN, NULL));
}

void WeatherPlugin::masterDisconnected() {
	// when the master connects, logout from the FritzBox
	//this->queue.enqueueNotification(new ActionNotification(ActionNotification::LOGOUT, NULL));
}

void WeatherPlugin::run(void) {

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": WeatherPlugin worker thread started");

	while (true) {
		try {

		} catch (Poco::Exception &e) {
			this->opdid->log(this->nodeID + ": Unhandled exception in worker thread: " + e.message());
		}
	}

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": WeatherPlugin worker thread terminated");
}

// plugin instance factory function

#ifdef _WINDOWS

extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#elif linux

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#endif

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the WeatherPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new WeatherPlugin();
}

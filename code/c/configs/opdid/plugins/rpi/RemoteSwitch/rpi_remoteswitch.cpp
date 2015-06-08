
// OPDID plugin that supports 433 MHz radio controlled power sockets on Raspberry Pi

#include "Poco/Tuple.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "WiringPi.h"
#include "RCSwitch.h"

#include "LinuxOPDID.h"

///////////////////////////////////////////////////////////////////////////////
// RemoteSwitch: Plugin for remote power outlet control
///////////////////////////////////////////////////////////////////////////////

class RemoteSwitchPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;
	std::string nodeID;

	int gpioPin;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};

///////////////////////////////////////////////////////////////////////////////
// RemoteSwitchPort: Represents a remote-controlled power outlet
// The RemoteSwitchPort is a SelectPort that will never report its state.
// This is because radio power outlets do not have a way to query their state;
// thus we don't know what's really going on with the device.
///////////////////////////////////////////////////////////////////////////////

class RemoteSwitchPort : public OPDI_SelectPort {
protected:
	AbstractOPDID *opdid;
	int code;
public:
	RemoteSwitchPort(AbstractOPDID *opdid, const char *ID, int code);
	virtual ~RemoteSwitchPort(void);
	virtual void setPosition(uint16_t position) override;
	virtual void getState(uint16_t *position) override;
};

RemoteSwitchPort::RemoteSwitchPort(AbstractOPDID *opdid, const char *ID, int code) : OPDI_SelectPort(ID, 
	(std::string("RemoteSwitchPort@") + to_string(pin)).c_str(), // default label - can be changed by configuration
	0) {
	this->opdid = opdid;
	this->code = code;
}

RemoteSwitchPort::~RemoteSwitchPort(void) {
	// release resources; configure as floating input
}

void RemoteSwitchPort::setPosition(uint16_t position)  {
	OPDI_SelectPort::setPosition(position);

	if (position == 0) {
		
	} else {
		
	}
}

void RemoteSwitchPort::getPosition(uint16_t *position) {
	// the position is always unknown
	*position = -1;
}


///////////////////////////////////////////////////////////////////////////////
// RemoteSwitchPlugin
///////////////////////////////////////////////////////////////////////////////

void RemoteSwitchPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;
	this->nodeID = node;

	Poco::Util::AbstractConfiguration *nodeConfig = config->createView(node);

	int pin = config->getInt("Pin", -1);
	if (pin < 0)
		throw Poco::DataException("You have to specify a pin for the 433 MHz remote control module data line");
	// TODO validate pin
	this->gpioPin = pin;
	
	this->opdid->lockResource(std::string("RemoteSwitchPlugin@") + to_string(this-gpioPin), node);

	// the remote switch plugin node expects a list of node names that determine the ports that this plugin provides

	// enumerate keys of the plugin's nodes (in specified order)
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("Enumerating RemoteSwitch nodes: " + node + ".Nodes");

	Poco::Util::AbstractConfiguration *nodes = config->createView(node + ".Nodes");

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");

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
			this->opdid->log("Setting up RemoteSwitch port) for node: " + nodeName);
			
		// get port section from the configuration
		Poco::Util::AbstractConfiguration *portConfig = config->createView(nodeName);
	
		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, "Type", "", true);

		if (portType == "RemoteSwitch") {
			// read code
			int code = portConfig->getInt("Code", -1);
			// check whether the code is valid
			if (code < 0)
				throw Poco::DataException("A 'Code' greater or equal than 0 must be specified for a RemoteSwitch port");

			// setup the port instance and add it; use internal pin number
			RemoteSwitchPort *port = new RemoteSwitchPort(abstractOPDID, nodeName.c_str(), code);
			// set default group: RemoteSwitchPlugin node's group
			port->setGroup(group);
			abstractOPDID->configureSelectPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		nli++;
	}
	
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("RemoteSwitchPlugin setup completed successfully as node " + node);
}

void RemoteSwitchPlugin::masterConnected() {
}

void RemoteSwitchPlugin::masterDisconnected() {
}


// plugin factory function
extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the RemoteSwitchPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new RemoteSwitchPlugin();
}

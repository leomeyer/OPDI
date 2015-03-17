
// OPDID plugin that supports the Gertboard on Raspberry Pi

#include "Poco/Tuple.h"

#include "opdi_constants.h"
#include "gb_common.h"

#include "LinuxOPDID.h"

// mapping to different revisions of RPi
// first number indicates Gertboard label, second specifies internal pin number
static int pinMapRev1[][2] = { 
	{0, 0}, {1, 1}, {4, 4}, {7, 7}, {8, 8}, {9, 9}, 
	{10, 10}, {11, 11}, {14, 14}, {15, 15}, {17, 17}, 
	{18, 18}, {21, 21}, {22, 22}, {23, 23}, {24, 24}, {25, 25},
	{-1, -1}
};

static int pinMapRev2[][2] = { 
	{0, 2}, {1, 3}, {4, 4}, {7, 7}, {8, 8}, {9, 9}, 
	{10, 10}, {11, 11}, {14, 14}, {15, 15}, {17, 17}, 
	{18, 18}, {21, 27}, {22, 22}, {23, 23}, {24, 24}, {25, 25},
	{-1, -1}
};

///////////////////////////////////////////////////////////////////////////////
// DigitalGertboardPort: Represents a digital input/output pin on the Gertboard.
///////////////////////////////////////////////////////////////////////////////

class DigitalGertboardPort : public OPDI_DigitalPort {
protected:
	int pin;
public:
	DigitalGertboardPort(const char *ID, int pin);
	virtual uint8_t setLine(uint8_t line) override;
	virtual uint8_t setMode(uint8_t mode) override;
	virtual uint8_t getState(uint8_t *mode, uint8_t *line) override;
};

DigitalGertboardPort::DigitalGertboardPort(const char *ID, int pin) : OPDI_DigitalPort(ID, 
	(std::string("Digital Gertboard Port ") + to_string(pin)).c_str(), // default label - can be changed by configuration
	OPDI_PORTDIRCAP_INPUT,	// default: input
	0) {
	this->pin = pin;
}

uint8_t DigitalGertboardPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);

	AbstractOPDID *opdid = (AbstractOPDID *)this->opdi;
	if (line == 0) {
		GPIO_CLR0 = (1 << this->pin);
		
		if (opdid->logVerbosity == AbstractOPDID::VERBOSE)
			opdid->log("DigitalGertboardPort line set to Low of internal pin: " + to_string(this->pin));
	} else {
		GPIO_SET0 = (1 << this->pin);
		
		if (opdid->logVerbosity == AbstractOPDID::VERBOSE)
			opdid->log("DigitalGertboardPort line set to High of internal pin: " + to_string(this->pin));
	}

	return OPDI_STATUS_OK;
}

uint8_t DigitalGertboardPort::setMode(uint8_t mode) {

	// cannot set pullup/pulldown modes
	if ((mode == OPDI_DIGITAL_MODE_INPUT_PULLUP) || (mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN))
		throw PortError("Digital Gertboard Port does not support pullup/pulldown resistor modes");

	OPDI_DigitalPort::setMode(mode);
	
	if (this->mode == 0) {
		// configure as input
		INP_GPIO(this->pin);
	} else {
		// configure as output
		INP_GPIO(this->pin); 
		OUT_GPIO(this->pin);
	}
	
	return OPDI_STATUS_OK;	
}

uint8_t DigitalGertboardPort::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;
	
	// read line
	unsigned int b = GPIO_IN0;
	// bit set?
	*line = (b & (1 << this->pin)) == (unsigned int)(1 << this->pin) ? 1 : 0;

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// GertboardOPDIDPlugin: Plugin for providing Gertboard resources to OPDID
///////////////////////////////////////////////////////////////////////////////

class GertboardOPDIDPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};


void GertboardOPDIDPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;

	// prepare Gertboard IO (requires root permissions)
	setup_io();

	// the Gertboard plugin node expects a list of node names that determine the ports that this plugin provides

	// enumerate keys of the plugin's node (in specified order)
	// get ordered list of ports
	Poco::Util::AbstractConfiguration::Keys portKeys;
	nodeConfig->keys("", portKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of port keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = portKeys.begin(); it != portKeys.end(); ++it) {
	
		// there will be an item "Driver"; ignore it
		if (*it == "Driver")
			continue;
	
		int itemNumber = nodeConfig->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber <= 0)
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
		this->opdid->log("Warning: No Gertboard ports configured for this node; is this intended? Node: " + node);
	}
	
	// determine pin map to use
	int (*pinMap)[][2] = (int (*)[][2])&pinMapRev1;
	std::string revision = abstractOPDID->getConfigString(nodeConfig, "Revision", "1", false);
	if (revision == "1") {
		// default
	} else
	if (revision == "2") {
		pinMap = (int (*)[][2])&pinMapRev2;
	} else
		throw Poco::DataException("Gertboard revision not supported (use 1 or 2); Revision = " + revision);
	
	// go through items, create ports in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {
	
		if (this->opdid->logVerbosity == AbstractOPDID::VERBOSE)
			this->opdid->log("Setting up Gertboard port(s) for node: " + nli->get<1>());
			
		// get port section from the configuration
		Poco::Util::AbstractConfiguration *portConfig = abstractOPDID->getConfiguration()->createView(nli->get<1>());
	
		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, "Type", "", true);

		if (portType == "DigitalPort") {
			// read pin number
			int pinNumber = portConfig->getInt("Pin", 0);
			// check whether the pin is valid; determine internal pin
			if (pinNumber <= 0)
				continue;
			int i = 0;
			int internalPin = -1;
			// linear search through pin definitions; map to internal pin if found
			while ((*pinMap)[i][0] >= 0) {
				if ((*pinMap)[i][0] == pinNumber) {
					internalPin = (*pinMap)[i][1];
					break;
				}
				i++;
			}
			if (internalPin < 0)
				throw Poco::DataException(std::string("The pin is not supported: ") + abstractOPDID->to_string(pinNumber));

			// setup the port instance and add it; use internal pin number
			DigitalGertboardPort *port = new DigitalGertboardPort(nli->get<1>().c_str(), internalPin);
			abstractOPDID->configureDigitalPort(portConfig, port);
			abstractOPDID->addPort(port);
		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		nli++;
	}
	
	if (this->opdid->logVerbosity == AbstractOPDID::VERBOSE)
		this->opdid->log("GertboardOPDIDPlugin setup completed successfully as node " + node);
}

void GertboardOPDIDPlugin::masterConnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->log("Test plugin: master connected");
}

void GertboardOPDIDPlugin::masterDisconnected() {
	if (this->opdid->logVerbosity != AbstractOPDID::QUIET)
		this->opdid->log("Test plugin: master disconnected");
}

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the GertboardOPDIDPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new GertboardOPDIDPlugin();
}

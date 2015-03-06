
#include <vector>

#include "Poco/Exception.h"
#include "Poco/Tuple.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"
#include "OPDI_Ports.h"

#define DEFAULT_IDLETIMEOUT_MS	180000
#define DEFAULT_TCP_PORT		13110


// slave protocoll callback
void protocol_callback(uint8_t state) {
	Opdi->protocolCallback(state);
}


AbstractOPDID::AbstractOPDID(void) {
	this->logVerbosity = NORMAL;
	this->configuration = NULL;
}

AbstractOPDID::~AbstractOPDID(void) {
}

void AbstractOPDID::protocolCallback(uint8_t protState) {
	if (protState == OPDI_PROTOCOL_START_HANDSHAKE) {
		if (this->logVerbosity != AbstractOPDID::QUIET)
			this->log("Handshake started");
	} else
	if (protState == OPDI_PROTOCOL_CONNECTED) {
		this->connected(opdi_master_name);
	} else
	if (protState == OPDI_PROTOCOL_DISCONNECTED) {
		this->disconnected();
	}
}

void AbstractOPDID::connected(const char *masterName) {
	this->masterName = std::string(masterName);

	if (this->logVerbosity != AbstractOPDID::QUIET)
		this->log("Connected to: " + this->masterName);
	
	// notify registered listeners
	ConnectionListenerList::iterator it = this->connectionListeners.begin();
	while (it != this->connectionListeners.end()) {
		(*it)->masterConnected();
		it++;
	}
}

void AbstractOPDID::disconnected() {
	if (this->logVerbosity != AbstractOPDID::QUIET)
		this->log("Disconnected from: " + this->masterName);
	
	this->masterName = std::string();

	// notify registered listeners
	ConnectionListenerList::iterator it = this->connectionListeners.begin();
	while (it != this->connectionListeners.end()) {
		(*it)->masterDisconnected();
		it++;
	}
}

void AbstractOPDID::addConnectionListener(IOPDIDConnectionListener *listener) {
	this->removeConnectionListener(listener);
	this->connectionListeners.push_back(listener);
}

void AbstractOPDID::removeConnectionListener(IOPDIDConnectionListener *listener) {
	ConnectionListenerList::iterator it = std::find(this->connectionListeners.begin(), this->connectionListeners.end(), listener);
	if (it != this->connectionListeners.end())
		this->connectionListeners.erase(it);
}

void AbstractOPDID::sayHello(void) {
	if (this->logVerbosity == QUIET)
		return;

	this->log("OPDID version 0.1 (c) Leo Meyer 2015");

	if (this->logVerbosity == VERBOSE) {
		this->log("Build: " + std::string(__DATE__) + " " + std::string(__TIME__));
	}
}

void AbstractOPDID::showHelp(void) {
	this->sayHello();
	this->println("OPDI (Open Protocol for Device Interaction) service");
	this->println("Mandatory command line parameters:");
	this->println("  -c <config_file>: use the specified configuration file");
	this->println("Optional command line parameters:");
	this->println("  --version: print version number and exit");
	this->println("  -h or -?: print help text and exit");
	this->println("  -q: quiet mode (print errors only)");
	this->println("  -v: verbose mode");
}

std::string AbstractOPDID::getTimestampStr(void) {
	return "[" + Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%Y-%m-%d %H:%M:%S.%i") + "] ";
}

void AbstractOPDID::readConfiguration(const std::string filename) {
	// will throw exceptions if something goes wrong
	// expect ini file; TODO: support other file formats
	configuration = new Poco::Util::IniFileConfiguration(filename);
}

std::string AbstractOPDID::getConfigString(Poco::Util::AbstractConfiguration *config, const std::string &key, const std::string &defaultValue, const bool isRequired) {
	if (isRequired) {
		if (!config->hasProperty(key)) {
			throw Poco::DataException("Expected configuration parameter not found", key);
		}
	}
	return config->getString(key, defaultValue);
}

Poco::Util::AbstractConfiguration *AbstractOPDID::getConfiguration() {
	return this->configuration;
}

void AbstractOPDID::println(std::string text) {
	this->println(text.c_str());
}

void AbstractOPDID::log(std::string text) {
	this->println(this->getTimestampStr() + text);
}

int AbstractOPDID::startup(std::vector<std::string> args) {
	// evaluate arguments
	for (unsigned int i = 0; i < args.size(); i++) {
		if (args.at(i) == "-h" || args.at(i) == "-?") {
			this->showHelp();
			return 0;
		}
		if (args.at(i) == "--version") {
			this->logVerbosity = VERBOSE;
			this->sayHello();
			return 0;
		}
		if (args.at(i) == "-v") {
			this->logVerbosity = VERBOSE;
		}
		if (args.at(i) == "-q") {
			this->logVerbosity = QUIET;
		}
		if (args.at(i) == "-c") {
			i++;
			if (args.size() == i) {
				throw Poco::SyntaxException("Expected configuration file name after argument -c");
			} else {
				this->readConfiguration(args.at(i));
			}
		}
	}

	// no configuration?
	if (this->configuration == NULL)
		throw Poco::SyntaxException("Expected argument: -c <config_file>");

	// create view to "General" section
	Poco::Util::AbstractConfiguration *general = this->configuration->createView("General");
	this->setGeneralConfiguration(general);

	Opdi->sayHello(); 

	this->setupNodes(this->configuration);

	// create view to "Connection" section
	Poco::Util::AbstractConfiguration *connection = this->configuration->createView("Connection");

	return this->setupConnection(connection);
}

void AbstractOPDID::setGeneralConfiguration(Poco::Util::AbstractConfiguration *general) {
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up general configuration");

	std::string slaveName = this->getConfigString(general, "SlaveName", "", true);
	int timeout = general->getInt("IdleTimeout", DEFAULT_IDLETIMEOUT_MS);
	std::string logVerbosityStr = this->getConfigString(general, "LogVerbosity", "", false);

	if (logVerbosityStr != "") { 
		if (logVerbosityStr == "Quiet") {
			this->logVerbosity = QUIET;
		} else
		if (logVerbosityStr == "Normal") {
			this->logVerbosity = NORMAL;
		} else
		if (logVerbosityStr == "Verbose") {
			this->logVerbosity = VERBOSE;
		} else
			throw Poco::InvalidArgumentException("Verbosity level unknown (expected one of 'Quiet', 'Normal', or 'Verbose')", logVerbosityStr);
	}

	// initialize OPDI slave
	this->setup(slaveName.c_str(), timeout);
}

void AbstractOPDID::configureDigitalPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DigitalPort *port) {

	std::string portLabel = this->getConfigString(portConfig, "Label", "", true);
	port->setLabel(portLabel.c_str());

	std::string portDirCaps = this->getConfigString(portConfig, "DirCaps", "Bidi", false);
	const char *dircaps;
	if (portDirCaps == "Input") {
		dircaps = OPDI_PORTDIRCAP_INPUT;
	} else if (portDirCaps == "Output") {
		dircaps = OPDI_PORTDIRCAP_OUTPUT;
	} else if (portDirCaps == "Bidi") {
		dircaps = OPDI_PORTDIRCAP_BIDI;
	} else
		throw Poco::DataException("Unknown port DirCaps specifier; expected 'Input', 'Output' or 'Bidi'", portDirCaps);
	port->setDirCaps(dircaps);

	uint8_t flags = portConfig->getInt("Flags", 0);
	port->flags = flags;
}

void AbstractOPDID::setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up emulated digital port: " + port);

	OPDI_DigitalPort *digPort = new OPDI_DigitalPort(port.c_str());
	this->configureDigitalPort(portConfig, digPort);

	this->addPort(digPort);
}

void AbstractOPDID::configureAnalogPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_AnalogPort *port) {
	std::string portLabel = this->getConfigString(portConfig, "Label", "", true);
	port->setLabel(portLabel.c_str());

	std::string portDirCaps = this->getConfigString(portConfig, "DirCaps", "Bidi", false);
	const char *dircaps;
	if (portDirCaps == "Input") {
		dircaps = OPDI_PORTDIRCAP_INPUT;
	} else if (portDirCaps == "Output") {
		dircaps = OPDI_PORTDIRCAP_OUTPUT;
	} else if (portDirCaps == "Bidi") {
		dircaps = OPDI_PORTDIRCAP_BIDI;
	} else
		throw Poco::DataException("Unknown port DirCaps specifier; expected 'Input', 'Output' or 'Bidi'", portDirCaps);
	port->setDirCaps(dircaps);

	// set initial values
	if (portConfig->hasProperty("Mode")) {
		std::string mode = this->getConfigString(portConfig, "Mode", "Output", false);
		if (mode == "Input")
			port->setMode(0);
		else if (mode == "Output")
			port->setMode(1);
		else
			throw Poco::ApplicationException("Unknown analog port mode, expected 'Input' or 'Output'", mode);
	} else
		// default mode: output
		port->setMode(1);

	if (portConfig->hasProperty("Resolution")) {
		uint8_t resolution = portConfig->getInt("Resolution", OPDI_ANALOG_PORT_RESOLUTION_12);
		if (port->setResolution(resolution) != OPDI_STATUS_OK) {
			throw Poco::ApplicationException("Unable to set resolution from configuration", this->to_string(resolution));
		}
	}
	if (portConfig->hasProperty("Value")) {
		uint32_t value = portConfig->getInt("Value", 0);
		if (port->setValue(value) != OPDI_STATUS_OK) {
			throw Poco::ApplicationException("Unable to set value from configuration", this->to_string(value));
		}
	}

	uint8_t flags = portConfig->getInt("Flags", 0);
	port->flags = flags;
}

void AbstractOPDID::setupEmulatedAnalogPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up emulated analog port: " + port);

	OPDI_AnalogPort *anaPort = new OPDI_AnalogPort(port.c_str());
	this->configureAnalogPort(portConfig, anaPort);

	this->addPort(anaPort);
}

void AbstractOPDID::configureSelectPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_SelectPort *port) {
	std::string portLabel = this->getConfigString(portConfig, "Label", "", true);
	port->setLabel(portLabel.c_str());

	// the select port requires a prefix or section "<portID>.Items"
	Poco::Util::AbstractConfiguration *portItems = this->configuration->createView(std::string(port->getID()) + ".Items");

	// get ordered list of items
	Poco::Util::AbstractConfiguration::Keys itemKeys;
	portItems->keys("", itemKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of items (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = itemKeys.begin(); it != itemKeys.end(); ++it) {
		int itemNumber = portItems->getInt(*it, 0);
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

	if (orderedItems.size() == 0)
		throw Poco::DataException("The select port " + std::string(port->getID()) + " requires at least one item in its config section", std::string(port->getID()) + ".Items");

	// go through items, create ordered list of char* items
	std::vector<const char*> charItems;
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {
		charItems.push_back(nli->get<1>().c_str());
		nli++;
	}
	charItems.push_back(NULL);

	// set port items
	port->setItems(&charItems[0]);
}

void AbstractOPDID::setupEmulatedSelectPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up emulated select port: " + port);

	OPDI_SelectPort *selPort = new OPDI_SelectPort(port.c_str());
	this->configureSelectPort(portConfig, selPort);

	this->addPort(selPort);
}

void AbstractOPDID::setupNode(Poco::Util::AbstractConfiguration *config, std::string node) {
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up node: " + node);

	// create node section view
	Poco::Util::AbstractConfiguration *nodeConfig = this->configuration->createView(node);

	// get node information
	std::string nodeType = this->getConfigString(nodeConfig, "Type", "", true);
	std::string nodeDriver = this->getConfigString(nodeConfig, "Driver", "", false);

	// driver specified?
	if (nodeDriver != "") {
		// try to load the plugin; the driver name is the (platform dependent) library file name
		IOPDIDPlugin *plugin = this->getPlugin(nodeDriver);

		// init the plugin
		plugin->setupPlugin(this, node, nodeConfig);
	} else {
		// standard driver (internal ports)
		if (nodeType == "DigitalPort") {
			this->setupEmulatedDigitalPort(nodeConfig, node);
		} else
		if (nodeType == "AnalogPort") {
			this->setupEmulatedAnalogPort(nodeConfig, node);
		} else
		if (nodeType == "SelectPort") {
			this->setupEmulatedSelectPort(nodeConfig, node);
		} else
			throw Poco::DataException("Invalid configuration: Unknown node type", nodeType);
	}
}

void AbstractOPDID::setupNodes(Poco::Util::AbstractConfiguration *config) {
	// enumerate section "Nodes"
	Poco::Util::AbstractConfiguration *nodes = this->configuration->createView("Nodes");
	if (this->logVerbosity == VERBOSE)
		this->log("Setting up nodes");

	Poco::Util::AbstractConfiguration::Keys nodeKeys;
	nodes->keys("", nodeKeys);

	typedef Poco::Tuple<int, std::string> Node;
	typedef std::vector<Node> NodeList;
	NodeList orderedNodes;

	// create ordered list of nodes (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = nodeKeys.begin(); it != nodeKeys.end(); ++it) {
		int nodeNumber = nodes->getInt(*it, 0);
		// check whether the node is active
		if (nodeNumber <= 0)
			continue;

		// insert at the correct position to create a sorted list of nodes
		NodeList::iterator nli = orderedNodes.begin();
		while (nli != orderedNodes.end()) {
			if (nli->get<0>() > nodeNumber)
				break;
			nli++;
		}
		Node node(nodeNumber, *it);
		orderedNodes.insert(nli, node);
	}

	// go through ordered list, setup nodes by name
	NodeList::const_iterator nli = orderedNodes.begin();
	while (nli != orderedNodes.end()) {
		this->setupNode(config, nli->get<1>());
		nli++;
	}
}

int AbstractOPDID::setupConnection(Poco::Util::AbstractConfiguration *config) {

	if (this->logVerbosity == VERBOSE)
		this->log("Setting up connection");
	std::string connectionType = this->getConfigString(config, "Type", "", true);

	if (connectionType == "TCP") {
		std::string interface_ = this->getConfigString(config, "Interface", "*", false);
		int port = config->getInt("Port", DEFAULT_TCP_PORT);

		return this->setupTCP(interface_, port);
	}
	else
		throw Poco::DataException("Invalid configuration; unknown connection type", connectionType);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following functions implement the glue code between C and C++.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Can be used to send debug messages to a monitor.
 *
 */
uint8_t opdi_debug_msg(const uint8_t *message, uint8_t direction) {
	if (Opdi->logVerbosity != AbstractOPDID::VERBOSE)
		return OPDI_STATUS_OK;
	std::string dirChar = "-";
	if (direction == OPDI_DIR_INCOMING)
		dirChar = ">";
	else
	if (direction == OPDI_DIR_OUTGOING)
		dirChar = "<";
	else
	if (direction == OPDI_DIR_INCOMING_ENCR)
		dirChar = "}";
	else
	if (direction == OPDI_DIR_OUTGOING_ENCR)
		dirChar = "{";
	Opdi->log(dirChar + (const char *)message);
	return OPDI_STATUS_OK;
}

const char *opdi_encoding = new char[MAX_ENCODINGNAMELENGTH];
uint16_t opdi_device_flags = 0;
const char *opdi_supported_protocols = "BP";
const char *opdi_config_name = new char[MAX_SLAVENAMELENGTH];
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];

// digital port functions
#ifndef NO_DIGITAL_PORTS

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	uint8_t result;
	uint8_t dMode;
	uint8_t dLine;
	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	result = dPort->getState(&dMode, &dLine);
	if (result != OPDI_STATUS_OK)
		return result;
	mode[0] = '0' + dMode;
	line[0] = '0' + dLine;

	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]) {
	uint8_t dLine;

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (line[0] == '1')
		dLine = 1;
	else
		dLine = 0;
	return dPort->setLine(dLine);
}

uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]) {
	uint8_t dMode;

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((mode[0] >= '0') && (mode[0] <= '3'))
		dMode = mode[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return dPort->setMode(dMode);
}

#endif 		// NO_DIGITAL_PORTS

#ifndef NO_ANALOG_PORTS

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	uint8_t result;
	uint8_t aMode;
	uint8_t aRef;
	uint8_t aRes;

	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	result = aPort->getState(&aMode, &aRes, &aRef, value);
	if (result != OPDI_STATUS_OK)
		return result;
	mode[0] = '0' + aMode;
	res[0] = '0' + (aRes - 8);
	ref[0] = '0' + aRef;

	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return aPort->setValue(value);
}

uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	uint8_t aMode;

	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((mode[0] >= '0') && (mode[0] <= '1'))
		aMode = mode[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setMode(aMode);
}

uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]) {
	uint8_t aRes;
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((res[0] >= '0') && (res[0] <= '4'))
		aRes = res[0] - '0' + 8;
	else
		// resolution not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setResolution(aRes);
}

uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]) {
	uint8_t aRef;
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((ref[0] >= '0') && (ref[0] <= '1'))
		aRef = ref[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setReference(aRef);
}

#endif	// NO_ANALOG_PORTS

#ifndef OPDI_NO_SELECT_PORTS

uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *position) {
	OPDI_SelectPort *sPort = (OPDI_SelectPort *)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return sPort->getState(position);
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position) {
	OPDI_SelectPort *sPort = (OPDI_SelectPort *)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return sPort->setPosition(position);
}

#endif	//  OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

uint8_t opdi_get_dial_port_state(opdi_Port *port, int32_t *position) {
	OPDI_DialPort *dPort = (OPDI_DialPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return dPort->getState(position);
}

uint8_t opdi_set_dial_port_position(opdi_Port *port, int32_t position) {
	OPDI_DialPort *dPort = (OPDI_DialPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return dPort->setPosition(position);
}

#endif	// OPDI_NO_DIAL_PORTS

uint8_t opdi_choose_language(const char *languages) {
	// TODO

	return OPDI_STATUS_OK;
}

#ifdef OPDI_HAS_MESSAGE_HANDLED

uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	return Opdi->messageHandled(channel, parts);
}

#endif

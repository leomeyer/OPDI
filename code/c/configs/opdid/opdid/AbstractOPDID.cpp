
#include <vector>
#include <time.h>

#include "Poco/Exception.h"
#include "Poco/Tuple.h"
#include "Poco/Timestamp.h"
#include "Poco/Timezone.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeParser.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/Mutex.h"
#include "Poco/SimpleFileChannel.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"
#include "OPDI_Ports.h"
#include "OPDID_Ports.h"

#define DEFAULT_IDLETIMEOUT_MS	180000
#define DEFAULT_TCP_PORT		13110

#define OPDID_CONFIG_FILE_SETTING	"__OPDID_CONFIG_FILE_PATH"

// slave protocoll callback
void protocol_callback(uint8_t state) {
	Opdi->protocolCallback(state);
}

AbstractOPDID::AbstractOPDID(void) {
	this->majorVersion = OPDID_MAJOR_VERSION;
	this->minorVersion = OPDID_MINOR_VERSION;
	this->patchVersion = OPDID_PATCH_VERSION;

	this->logVerbosity = UNKNOWN;
	this->configuration = NULL;

	this->logger = NULL;
	this->timestampFormat = "%Y-%m-%d %H:%M:%S.%i";

	// map result codes
	opdiCodeTexts[0] = "STATUS_OK";
	opdiCodeTexts[1] = "DISCONNECTED";
	opdiCodeTexts[2] = "TIMEOUT";
	opdiCodeTexts[3] = "CANCELLED";
	opdiCodeTexts[4] = "ERROR_MALFORMED_MESSAGE";
	opdiCodeTexts[5] = "ERROR_CONVERSION";
	opdiCodeTexts[6] = "ERROR_MSGBUF_OVERFLOW";
	opdiCodeTexts[7] = "ERROR_DEST_OVERFLOW";
	opdiCodeTexts[8] = "ERROR_STRINGS_OVERFLOW";
	opdiCodeTexts[9] = "ERROR_PARTS_OVERFLOW";
	opdiCodeTexts[10] = "PROTOCOL_ERROR";
	opdiCodeTexts[11] = "PROTOCOL_NOT_SUPPORTED";
	opdiCodeTexts[12] = "ENCRYPTION_NOT_SUPPORTED";
	opdiCodeTexts[13] = "ENCRYPTION_REQUIRED";
	opdiCodeTexts[14] = "ENCRYPTION_ERROR";
	opdiCodeTexts[15] = "AUTH_NOT_SUPPORTED";
	opdiCodeTexts[16] = "AUTHENTICATION_EXPECTED";
	opdiCodeTexts[17] = "AUTHENTICATION_FAILED";
	opdiCodeTexts[18] = "DEVICE_ERROR";
	opdiCodeTexts[19] = "TOO_MANY_PORTS";
	opdiCodeTexts[20] = "PORTTYPE_UNKNOWN";
	opdiCodeTexts[21] = "PORT_UNKNOWN";
	opdiCodeTexts[22] = "WRONG_PORT_TYPE";
	opdiCodeTexts[23] = "TOO_MANY_BINDINGS";
	opdiCodeTexts[24] = "NO_BINDING";
	opdiCodeTexts[25] = "CHANNEL_INVALID";
	opdiCodeTexts[26] = "POSITION_INVALID";
	opdiCodeTexts[27] = "NETWORK_ERROR";
	opdiCodeTexts[28] = "TERMINATOR_IN_PAYLOAD";
	opdiCodeTexts[29] = "PORT_ACCESS_DENIED";
	opdiCodeTexts[30] = "PORT_ERROR";
	opdiCodeTexts[31] = "SHUTDOWN";
	opdiCodeTexts[32] = "GROUP_UNKNOWN";
}

AbstractOPDID::~AbstractOPDID(void) {
}

void AbstractOPDID::protocolCallback(uint8_t protState) {
	if (protState == OPDI_PROTOCOL_START_HANDSHAKE) {
		if (this->logVerbosity >= AbstractOPDID::VERBOSE)
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

	this->log("OPDID version " + this->to_string(this->majorVersion) + "." + this->to_string(this->minorVersion) + "." + this->to_string(this->patchVersion) + " (c) Leo Meyer 2015");

	if (this->logVerbosity >= VERBOSE) {
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
	return Poco::DateTimeFormatter::format(Poco::LocalDateTime(), this->timestampFormat);
}

std::string AbstractOPDID::getOPDIResult(uint8_t code) {
	OPDICodeTexts::const_iterator it = this->opdiCodeTexts.find(code);
	if (it == this->opdiCodeTexts.end())
		return std::string("Unknown status code: ") + to_string((int)code);
	return it->second;
}

Poco::Util::AbstractConfiguration *AbstractOPDID::readConfiguration(const std::string filename, std::map<std::string, std::string> parameters) {
	// will throw an exception if something goes wrong
	Poco::Util::AbstractConfiguration *result = new OPDIDConfigurationFile(filename, parameters);
	// remember config file location
	Poco::Path filePath(filename);
	result->setString(OPDID_CONFIG_FILE_SETTING, filePath.absolute().toString());

	return result;
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

void AbstractOPDID::printlne(std::string text) {
	this->printlne(text.c_str());
}

void AbstractOPDID::log(std::string text) {

	// important: log must be thread-safe.
	// encapsulate in a mutex

	// Try to lock the mutex. If this does not work in time, it will throw
	// an exception. The calling thread should deal with this exception.
	Poco::Mutex::ScopedLock(this->mutex);
	std::string msg = "[" + this->getTimestampStr() + "] " + (this->shutdownRequested ? "<AFTER SHUTDOWN> " : "") + text;

	if (this->logger != NULL) {
		this->logger->information(msg);
	} else {
		this->println(msg);
	}
}

void AbstractOPDID::logWarning(std::string text) {
	// suppress warnings?
	if (this->logVerbosity == QUIET)
		return;

	// important: log must be thread-safe.
	// encapsulate in a mutex

	// Try to lock the mutex. If this does not work in time, it will throw
	// an exception. The calling thread should deal with this exception.
	Poco::Mutex::ScopedLock(this->mutex);

	std::string msg = "WARNING: " + text;
	if (this->logger != NULL) {
		this->logger->error(msg);
	}
	this->printlne("[" + this->getTimestampStr() + "] " + msg);
}

void AbstractOPDID::logError(std::string text) {

	// important: log must be thread-safe.
	// encapsulate in a mutex

	// Try to lock the mutex. If this does not work in time, it will throw
	// an exception. The calling thread should deal with this exception.
	Poco::Mutex::ScopedLock(this->mutex);

	std::string msg = "ERROR: " + text;
	if (this->logger != NULL) {
		this->logger->error(msg);
	}
	this->printlne("[" + this->getTimestampStr() + "] " + msg);
}

int AbstractOPDID::startup(std::vector<std::string> args, std::map<std::string, std::string> environment) {

	this->environment = environment;
	this->shutdownRequested = false;

	// evaluate arguments
	for (unsigned int i = 1; i < args.size(); i++) {
		if (args.at(i) == "-h" || args.at(i) == "-?") {
			this->showHelp();
			return 0;
		} else
		if (args.at(i) == "--version") {
			this->logVerbosity = VERBOSE;
			this->sayHello();
			return 0;
		} else
		if (args.at(i) == "-d") {
			this->logVerbosity = DEBUG;
		} else
		if (args.at(i) == "-e") {
			this->logVerbosity = EXTREME;
		} else
		if (args.at(i) == "-v") {
			this->logVerbosity = VERBOSE;
		} else
		if (args.at(i) == "-q") {
			this->logVerbosity = QUIET;
		} else
		if (args.at(i) == "-c") {
			i++;
			if (args.size() == i) {
				throw Poco::SyntaxException("Expected configuration file name after argument -c");
			} else {
				// load configuration, substituting environment parameters
				this->configuration = this->readConfiguration(args.at(i), environment);
			}
		} else
		if (args.at(i) == "-l") {
			i++;
			if (args.size() == i) {
				throw Poco::SyntaxException("Expected log file name after argument -l");
			} else {
				// initialize logger and channel
				Poco::AutoPtr<Poco::SimpleFileChannel> pChannel(new Poco::SimpleFileChannel);
				pChannel->setProperty("path", args.at(i));
				pChannel->setProperty("rotation", "1 M");
				Poco::Logger::root().setChannel(pChannel);
				this->logger = Poco::Logger::has("");
			}
		} else {
			throw Poco::SyntaxException("Invalid argument", args.at(i));
		} 
	}

	// no configuration?
	if (this->configuration == NULL)
		throw Poco::SyntaxException("Expected argument: -c <config_file>");

	this->sayHello();

	// create view to "General" section
	Poco::Util::AbstractConfiguration *general = this->configuration->createView("General");
	this->setGeneralConfiguration(general);

	this->setupRoot(this->configuration);

	if (this->logVerbosity >= AbstractOPDID::VERBOSE)
		this->log("Node setup complete, preparing ports");

	this->preparePorts();

	// create view to "Connection" section
	Poco::Util::AbstractConfiguration *connection = this->configuration->createView("Connection");

	return this->setupConnection(connection);
}

void AbstractOPDID::lockResource(std::string resourceID, std::string lockerID) {
	if (this->logVerbosity >= AbstractOPDID::DEBUG)
		this->log("Trying to lock resource '" + resourceID + "' for " + lockerID);
	// try to locate the resource ID
	LockedResources::const_iterator it = this->lockedResources.find(resourceID);
	// resource already used?
	if (it != this->lockedResources.end())
		throw Poco::DataException("Resource requested by " + lockerID + " is already in use by " + it->second + ": " + resourceID);

	// store the resource
	this->lockedResources[resourceID] = lockerID;
}

void AbstractOPDID::setGeneralConfiguration(Poco::Util::AbstractConfiguration *general) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up general configuration");

	std::string slaveName = this->getConfigString(general, "SlaveName", "", true);
	int messageTimeout = general->getInt("MessageTimeout", OPDI_DEFAULT_MESSAGE_TIMEOUT);
	if ((messageTimeout < 0) || (messageTimeout > 65535))
			throw Poco::InvalidArgumentException("MessageTimeout must be greater than 0 and may not exceed 65535", to_string(messageTimeout));
	opdi_set_timeout(messageTimeout);

	int idleTimeout = general->getInt("IdleTimeout", DEFAULT_IDLETIMEOUT_MS);

	std::string logVerbosityStr = this->getConfigString(general, "LogVerbosity", "", false);

	// set log verbosity only if it's not already set
	if ((this->logVerbosity == UNKNOWN) && (logVerbosityStr != "")) {
		if (logVerbosityStr == "Quiet") {
			this->logVerbosity = QUIET;
		} else
		if (logVerbosityStr == "Normal") {
			this->logVerbosity = NORMAL;
		} else
		if (logVerbosityStr == "Verbose") {
			this->logVerbosity = VERBOSE;
		} else
		if (logVerbosityStr == "Debug") {
			this->logVerbosity = DEBUG;
		} else
		if (logVerbosityStr == "Extreme") {
			this->logVerbosity = EXTREME;
		} else
			throw Poco::InvalidArgumentException("Verbosity level unknown (expected one of 'Quiet', 'Normal', 'Verbose', 'Debug' or 'Extreme')", logVerbosityStr);
	}

	if (this->logVerbosity == UNKNOWN) {
		this->logVerbosity = NORMAL;
	}

	this->allowHiddenPorts = general->getBool("AllowHidden", true);

	// initialize OPDI slave
	this->setup(slaveName.c_str(), idleTimeout);
}

	/** Reads common properties from the configuration and configures the port group. */
void AbstractOPDID::configureGroup(Poco::Util::AbstractConfiguration *groupConfig, OPDI_PortGroup *group, int defaultFlags) {
	// the default label is the port ID
	std::string portLabel = this->getConfigString(groupConfig, "Label", group->getID(), false);
	group->setLabel(portLabel.c_str());

	int flags = groupConfig->getInt("Flags", -1);
	if (flags >= 0) {
		group->setFlags(flags);
	} else
		// default flags specified?
		// avoid calling setFlags unnecessarily because subclasses may implement specific behavior
		if (defaultFlags > 0)
			group->setFlags(defaultFlags);

	// extended properties
	std::string icon = this->getConfigString(groupConfig, "Icon", "", false);
	if (icon != "") {
		group->setIcon(icon);
	}
	std::string parent = this->getConfigString(groupConfig, "Parent", "", false);
	if (parent != "") {
		group->setParent(parent.c_str());
	}
}

void AbstractOPDID::setupGroup(Poco::Util::AbstractConfiguration *groupConfig, std::string group) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up group: " + group);

	OPDI_PortGroup *portGroup = new OPDI_PortGroup(group.c_str());
	this->configureGroup(groupConfig, portGroup, 0);

	this->addPortGroup(portGroup);
}

void AbstractOPDID::setupInclude(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig, std::string node) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up include: " + node);

	// filename must be present
	std::string filename = this->getConfigString(config, "Filename", "", true);

	// determine path type; default: relative to parent config file
	std::string relativeTo = this->getConfigString(config, "RelativeTo", "Config", false);

	if (relativeTo == "Application") {
		// nothing to do
	} else
	if (relativeTo == "Config") {
		// determine application-relative path depending on location of previous config file
		std::string configFilePath = config->getString(OPDID_CONFIG_FILE_SETTING, "");
		if (configFilePath == "")
			throw Poco::DataException("Programming error: Configuration file path not specified in config settings");

		Poco::Path filePath(configFilePath);
		Poco::Path absPath(filePath.absolute());
		Poco::Path parentPath = absPath.parent();
		// append or replace new config file path to path of previous config file
		Poco::Path finalPath = parentPath.resolve(filename);
		filename = finalPath.toString();
	} else
		throw Poco::DataException("Unknown RelativeTo property specified; expected 'Application' or 'Config'", relativeTo);

	// read parameters and build a map, based on the environment parameters
	std::map<std::string, std::string> parameters = this->environment;

	// the include node requires a section "<node>.Parameters"
	if (this->logVerbosity >= VERBOSE)
		this->log(node + ": Evaluating include parameters section: " + node + ".Parameters");

	Poco::Util::AbstractConfiguration *paramConfig = parentConfig->createView(node + ".Parameters");

	// get list of parameters
	Poco::Util::AbstractConfiguration::Keys paramKeys;
	paramConfig->keys("", paramKeys);

	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = paramKeys.begin(); it != paramKeys.end(); ++it) {
		std::string param = paramConfig->getString(*it, "");
		// store in the map
		parameters[*it] = param;
	}

	// warn if no parameters
	if ((parameters.size() == 0) && (this->logVerbosity >= NORMAL))
		this->log(node + ": No parameters for include in section " + node + ".Parameters found, is this intended?");

	if (this->logVerbosity >= VERBOSE)
		this->log(node + ": Processing include file: " + filename);

	Poco::Util::AbstractConfiguration *includeConfig = this->readConfiguration(filename, parameters);

	// setup the root node of the included configuration
	this->setupRoot(includeConfig);

	if (this->logVerbosity >= VERBOSE) {
		this->log(node + ": Include file " + filename + " processed successfully.");
		std::string configFilePath = parentConfig->getString(OPDID_CONFIG_FILE_SETTING, "");
		if (configFilePath != "")
			this->log("Continuing with parent configuration file: " + configFilePath);
	}
}

void AbstractOPDID::configurePort(Poco::Util::AbstractConfiguration *portConfig, OPDI_Port *port, int defaultFlags) {
	// ports can be hidden if allowed
	if (this->allowHiddenPorts)
		port->setHidden(portConfig->getBool("Hidden", false));

	// ports can be readonly
	port->setReadonly(portConfig->getBool("Readonly", false));

	// the default label is the port ID
	std::string portLabel = this->getConfigString(portConfig, "Label", port->getID(), false);
	port->setLabel(portLabel.c_str());

	std::string portDirCaps = this->getConfigString(portConfig, "DirCaps", "", false);
	if (portDirCaps == "Input") {
		port->setDirCaps(OPDI_PORTDIRCAP_INPUT);
	} else if (portDirCaps == "Output") {
		port->setDirCaps(OPDI_PORTDIRCAP_OUTPUT);
	} else if (portDirCaps == "Bidi") {
		port->setDirCaps(OPDI_PORTDIRCAP_BIDI);
	} else if (portDirCaps != "")
		throw Poco::DataException("Unknown DirCaps specified; expected 'Input', 'Output' or 'Bidi'", portDirCaps);

	int flags = portConfig->getInt("Flags", -1);
	if (flags >= 0) {
		port->setFlags(flags);
	} else
		// default flags specified?
		// avoid calling setFlags unnecessarily because subclasses may implement specific behavior
		if (defaultFlags > 0)
			port->setFlags(defaultFlags);

	std::string refreshMode = this->getConfigString(portConfig, "RefreshMode", "", false);
	if (refreshMode == "Off") {
		port->setRefreshMode(OPDI_Port::REFRESH_OFF);
	} else
	if (refreshMode == "Periodic") {
		port->setRefreshMode(OPDI_Port::REFRESH_PERIODIC);
	} else
	if (refreshMode == "Auto") {
		port->setRefreshMode(OPDI_Port::REFRESH_AUTO);
	} else
		if (refreshMode != "")
			throw Poco::DataException("Unknown RefreshMode specified; expected 'Off', 'Periodic' or 'Auto': " + refreshMode);

	if (port->getRefreshMode() == OPDI_Port::REFRESH_PERIODIC) {
		int time = portConfig->getInt("RefreshTime", -1);
		if (time >= 0) {
			port->setRefreshTime(time);
		} else {
			throw Poco::DataException("A RefreshTime > 0 must be specified in Periodic refresh mode: " + to_string(time));
		}
	}

	// extended properties
	std::string unit = this->getConfigString(portConfig, "Unit", "", false);
	if (unit != "") {
		port->setUnit(unit);
	}
	std::string icon = this->getConfigString(portConfig, "Icon", "", false);
	if (icon != "") {
		port->setIcon(icon);
	}
	std::string group = this->getConfigString(portConfig, "Group", "", false);
	if (group != "") {
		port->setGroup(group);
	}
}

void AbstractOPDID::configureDigitalPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DigitalPort *port) {
	this->configurePort(portConfig, port, 0);

	std::string portMode = this->getConfigString(portConfig, "Mode", "", false);
	if (portMode == "Input") {
		port->setMode(OPDI_DIGITAL_MODE_INPUT_FLOATING);
	} else if (portMode == "Input with pullup") {
		port->setMode(1);
	} else if (portMode == "Input with pulldown") {
		port->setMode(2);
	} else if (portMode == "Output") {
		port->setMode(3);
	} else if (portMode != "")
		throw Poco::DataException("Unknown Mode specified; expected 'Input', 'Input with pullup', 'Input with pulldown', or 'Output'", portMode);

	std::string portLine = this->getConfigString(portConfig, "Line", "", false);
	if (portLine == "High") {
		port->setLine(1);
	} else if (portLine == "Low") {
		port->setLine(0);
	} else if (portLine != "")
		throw Poco::DataException("Unknown Line specified; expected 'Low' or 'High'", portLine);
}

void AbstractOPDID::setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up emulated digital port: " + port);

	OPDI_DigitalPort *digPort = new OPDI_DigitalPort(port.c_str());
	this->configureDigitalPort(portConfig, digPort);

	this->addPort(digPort);
}

void AbstractOPDID::configureAnalogPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_AnalogPort *port) {
	this->configurePort(portConfig, port,
		// default flags: assume everything is supported
		OPDI_ANALOG_PORT_CAN_CHANGE_RES |
		OPDI_ANALOG_PORT_RESOLUTION_8 |
		OPDI_ANALOG_PORT_RESOLUTION_9 |
		OPDI_ANALOG_PORT_RESOLUTION_10 |
		OPDI_ANALOG_PORT_RESOLUTION_11 |
		OPDI_ANALOG_PORT_RESOLUTION_12 |
		OPDI_ANALOG_PORT_CAN_CHANGE_REF |
		OPDI_ANALOG_PORT_REFERENCE_INT |
		OPDI_ANALOG_PORT_REFERENCE_EXT);

	std::string mode = this->getConfigString(portConfig, "Mode", "", false);
	if (mode == "Input")
		port->setMode(0);
	else if (mode == "Output")
		port->setMode(1);
	else if (mode != "")
		throw Poco::ApplicationException("Unknown mode specified; expected 'Input' or 'Output'", mode);

	if (portConfig->hasProperty("Resolution")) {
		uint8_t resolution = portConfig->getInt("Resolution", OPDI_ANALOG_PORT_RESOLUTION_12);
		port->setResolution(resolution);
	}
	if (portConfig->hasProperty("Value")) {
		uint32_t value = portConfig->getInt("Value", 0);
		port->setValue(value);
	}
}

void AbstractOPDID::setupEmulatedAnalogPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up emulated analog port: " + port);

	OPDI_AnalogPort *anaPort = new OPDI_AnalogPort(port.c_str());
	this->configureAnalogPort(portConfig, anaPort);

	this->addPort(anaPort);
}

void AbstractOPDID::configureSelectPort(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig, OPDI_SelectPort *port) {
	this->configurePort(portConfig, port, 0);

	// the select port requires a prefix or section "<portID>.Items"
	Poco::Util::AbstractConfiguration *portItems = parentConfig->createView(std::string(port->getID()) + ".Items");

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

	uint16_t position = portConfig->getInt("Position", 0);
	if ((position < 0) || (position >= charItems.size()))
		throw Poco::DataException("Wrong select port setting: Position is out of range: " + to_string(position));
	port->setPosition(position);
}

void AbstractOPDID::setupEmulatedSelectPort(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up emulated select port: " + port);

	OPDI_SelectPort *selPort = new OPDI_SelectPort(port.c_str());
	this->configureSelectPort(portConfig, parentConfig, selPort);

	this->addPort(selPort);
}

void AbstractOPDID::configureDialPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DialPort *port) {
	this->configurePort(portConfig, port, 0);

	int64_t min = portConfig->getInt64("Min", 0);
	if (!portConfig->hasProperty("Max"))
		throw Poco::DataException("Missing dial port setting: Max");
	int64_t max = portConfig->getInt64("Max", 0);
	if (min >= max)
		throw Poco::DataException("Wrong dial port setting: Max must be greater than Min");
	int64_t step = portConfig->getInt64("Step", 1);
	if (step > (max - min))
		throw Poco::DataException("Wrong dial port setting: Step is too large: " + to_string(step));
	int64_t position = portConfig->getInt64("Position", min);
	if ((position < min) || (position > max))
		throw Poco::DataException("Wrong dial port setting: Position is out of range: " + to_string(position));

	port->setMin(min);
	port->setMax(max);
	port->setStep(step);
	port->setPosition(position);
}

void AbstractOPDID::setupEmulatedDialPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up emulated dial port: " + port);

	OPDI_DialPort *dialPort = new OPDI_DialPort(port.c_str());
	this->configureDialPort(portConfig, dialPort);

	this->addPort(dialPort);
}

void AbstractOPDID::configureStreamingPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_StreamingPort *port) {
	this->configurePort(portConfig, port, 0);
}

void AbstractOPDID::setupSerialStreamingPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up serial streaming port: " + port);

	OPDID_SerialStreamingPort *ssPort = new OPDID_SerialStreamingPort(this, port.c_str());
	ssPort->configure(portConfig);

	this->addPort(ssPort);
}

void AbstractOPDID::setupLoggingPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up logging port: " + port);

	OPDID_LoggingPort *logPort = new OPDID_LoggingPort(this, port.c_str());
	logPort->configure(portConfig);

	this->addPort(logPort);
}

void AbstractOPDID::setupLogicPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up LogicPort: " + port);

	OPDID_LogicPort *dlPort = new OPDID_LogicPort(this, port.c_str());
	dlPort->configure(portConfig);

	this->addPort(dlPort);
}

void AbstractOPDID::setupPulsePort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up PulsePort: " + port);

	OPDID_PulsePort *pulsePort = new OPDID_PulsePort(this, port.c_str());
	pulsePort->configure(portConfig);

	this->addPort(pulsePort);
}

void AbstractOPDID::setupSelectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up SelectorPort: " + port);

	OPDID_SelectorPort *selectorPort = new OPDID_SelectorPort(this, port.c_str());
	selectorPort->configure(portConfig);

	this->addPort(selectorPort);
}

#ifdef OPDID_USE_EXPRTK
void AbstractOPDID::setupExpressionPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up Expression: " + port);

	OPDID_ExpressionPort *expressionPort = new OPDID_ExpressionPort(this, port.c_str());
	expressionPort->configure(portConfig);

	this->addPort(expressionPort);
}
#endif	// def OPDID_USE_EXPRTK

void AbstractOPDID::setupTimerPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up Timer: " + port);

	OPDID_TimerPort *timerPort = new OPDID_TimerPort(this, port.c_str());
	timerPort->configure(portConfig);

	this->addPort(timerPort);
}

void AbstractOPDID::setupErrorDetectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up ErrorDetector: " + port);

	OPDID_ErrorDetectorPort *edPort = new OPDID_ErrorDetectorPort(this, port.c_str());
	edPort->configure(portConfig);

	this->addPort(edPort);
}

void AbstractOPDID::setupNode(Poco::Util::AbstractConfiguration *config, std::string node) {
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up node: " + node);

	// create node section view
	Poco::Util::AbstractConfiguration *nodeConfig = config->createView(node);

	// get node information
	std::string nodeDriver = this->getConfigString(nodeConfig, "Driver", "", false);

	// driver specified?
	if (nodeDriver != "") {
		if (this->logVerbosity >= VERBOSE)
			this->log("Loading plugin driver: " + nodeDriver);

		// try to load the plugin; the driver name is the (platform dependent) library file name
		IOPDIDPlugin *plugin = this->getPlugin(nodeDriver);

		// init the plugin
		plugin->setupPlugin(this, node, config);
	} else {
		std::string nodeType = this->getConfigString(nodeConfig, "Type", "", true);

		if (nodeType == "Group") {
			this->setupGroup(nodeConfig, node);
		} else 
		if (nodeType == "Include") {
			this->setupInclude(nodeConfig, config, node);
		} else 
		// standard driver (internal ports)
		if (nodeType == "DigitalPort") {
			this->setupEmulatedDigitalPort(nodeConfig, node);
		} else
		if (nodeType == "AnalogPort") {
			this->setupEmulatedAnalogPort(nodeConfig, node);
		} else
		if (nodeType == "SelectPort") {
			this->setupEmulatedSelectPort(nodeConfig, config, node);
		} else
		if (nodeType == "DialPort") {
			this->setupEmulatedDialPort(nodeConfig, node);
		} else
		if (nodeType == "SerialStreamingPort") {
			this->setupSerialStreamingPort(nodeConfig, node);
		} else
		if (nodeType == "LoggingPort") {
			this->setupLoggingPort(nodeConfig, node);
		} else
		if (nodeType == "LogicPort") {
			this->setupLogicPort(nodeConfig, node);
		} else
		if (nodeType == "PulsePort") {
			this->setupPulsePort(nodeConfig, node);
		} else
		if (nodeType == "SelectorPort") {
			this->setupSelectorPort(nodeConfig, node);
#ifdef OPDID_USE_EXPRTK
		} else
		if (nodeType == "ExpressionPort") {
			this->setupExpressionPort(nodeConfig, node);
#else
#pragma message( "Expression library not included, cannot use the ExpressionPort node type" )
#endif	// def OPDID_USE_EXPRTK
		} else
		if (nodeType == "Timer") {
			this->setupTimerPort(nodeConfig, node);
		} else
		if (nodeType == "ErrorDetector") {
			this->setupErrorDetectorPort(nodeConfig, node);
		} else
			throw Poco::DataException("Invalid configuration: Unknown node type", nodeType);
	}
}

void AbstractOPDID::setupRoot(Poco::Util::AbstractConfiguration *config) {
	// enumerate section "Root"
	Poco::Util::AbstractConfiguration *nodes = config->createView("Root");
	if (this->logVerbosity >= VERBOSE)
		this->log("Setting up root nodes");

	Poco::Util::AbstractConfiguration::Keys nodeKeys;
	nodes->keys("", nodeKeys);

	typedef Poco::Tuple<int, std::string> Node;
	typedef std::vector<Node> NodeList;
	NodeList orderedNodes;

	// create ordered list of nodes (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = nodeKeys.begin(); it != nodeKeys.end(); ++it) {
		int nodeNumber = nodes->getInt(*it, 0);
		// check whether the node is active
		if (nodeNumber < 0)
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

	// warn if no nodes found
	if ((orderedNodes.size() == 0) && (this->logVerbosity >= NORMAL)) {
		// get file name
		std::string filename = config->getString(OPDID_CONFIG_FILE_SETTING, "");
		if (filename == "")
			filename = "<unknown configuration file>";
		this->log(filename + ": No nodes in section Root found, is this intended?");
	}

	// go through ordered list, setup nodes by name
	NodeList::const_iterator nli = orderedNodes.begin();
	while (nli != orderedNodes.end()) {
		this->setupNode(config, nli->get<1>());
		nli++;
	}

	// TODO check group hierarchy
}

int AbstractOPDID::setupConnection(Poco::Util::AbstractConfiguration *config) {

	if (this->logVerbosity >= VERBOSE)
		this->log(std::string("Setting up connection for slave: ") + opdi_config_name);
	std::string connectionType = this->getConfigString(config, "Type", "", true);

	if (connectionType == "TCP") {
		std::string interface_ = this->getConfigString(config, "Interface", "*", false);
		int port = config->getInt("Port", DEFAULT_TCP_PORT);

		return this->setupTCP(interface_, port);
	}
	else
		throw Poco::DataException("Invalid configuration; unknown connection type", connectionType);
}

void AbstractOPDID::warnIfPluginMoreRecent(std::string driver) {
	// parse compile date and time
    std::stringstream compiled;
    compiled << __DATE__ << " " << __TIME__;

	Poco::DateTime buildTime;
	int tzd;

	if (!Poco::DateTimeParser::tryParse("%b %e %Y %H:%M:%s", compiled.str(), buildTime, tzd)) {
		if (this->logVerbosity >= AbstractOPDID::NORMAL)
			this->log("Warning: Could not parse build date and time to check possible ABI violation of driver " + driver + ": " + __DATE__ + " " + __TIME__);
	} else {
		// check whether the plugin driver file is more recent
		Poco::File driverFile(driver);
		// convert build time to UTC
		buildTime.makeUTC(Poco::Timezone::tzd());
		// assume both files have been built in the same time zone (getLastModified also returns UTC)
		if ((driverFile.getLastModified() < buildTime.timestamp()) && (this->logVerbosity >= AbstractOPDID::NORMAL))
			this->log("Warning: Plugin module " + driver + " is older than the main binary; possible ABI conflict! In case of strange errors please recompile the plugin!");
	}
}


uint8_t AbstractOPDID::waiting(uint8_t canSend) {

	// exception-safe processing
	try {
		return OPDI::waiting(canSend);
	} catch (Poco::Exception &pe) {
		this->log(std::string("Unhandled exception while housekeeping: ") + pe.message());
	} catch (std::exception &e) {
		this->log(std::string("Unhandled exception while housekeeping: ") + e.what());
	} catch (...) {
		this->log(std::string("Unknown error while housekeeping: "));
	}

	// TODO decide: ignore errors or abort?

	return OPDI_STATUS_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following functions implement the glue code between C and C++.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Can be used to send debug messages to a monitor.
 *
 */
uint8_t opdi_debug_msg(const char *message, uint8_t direction) {
	if (Opdi->logVerbosity < AbstractOPDID::DEBUG)
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
	Opdi->log(dirChar + message);
	return OPDI_STATUS_OK;
}

const char *opdi_encoding = new char[OPDI_MAX_ENCODINGNAMELENGTH];
uint16_t opdi_device_flags = 0;
const char *opdi_supported_protocols = "EP,BP";
const char *opdi_config_name = new char[OPDI_MAX_SLAVENAMELENGTH];
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];

// digital port functions
#ifndef NO_DIGITAL_PORTS

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	uint8_t dMode;
	uint8_t dLine;
	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	try {
		dPort->getState(&dMode, &dLine);
		mode[0] = '0' + dMode;
		line[0] = '0' + dLine;
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}

	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]) {
	uint8_t dLine;

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (dPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	if (line[0] == '1')
		dLine = 1;
	else
		dLine = 0;
	try {
		dPort->setLine(dLine);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]) {
	uint8_t dMode;

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (dPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	if ((mode[0] >= '0') && (mode[0] <= '3'))
		dMode = mode[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;
	try {
		dPort->setMode(dMode);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

#endif 		// NO_DIGITAL_PORTS

#ifndef NO_ANALOG_PORTS

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	uint8_t aMode;
	uint8_t aRef;
	uint8_t aRes;

	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	try {
		aPort->getState(&aMode, &aRes, &aRef, value);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	mode[0] = '0' + aMode;
	res[0] = '0' + (aRes - 8);
	ref[0] = '0' + aRef;

	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (aPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	try {
		aPort->setValue(value);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
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

	if (aPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	try {
		aPort->setMode(aMode);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
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

	try {
		aPort->setResolution(aRes);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
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

	try {
		aPort->setReference(aRef);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

#endif	// NO_ANALOG_PORTS

#ifndef OPDI_NO_SELECT_PORTS

uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *position) {
	OPDI_SelectPort *sPort = (OPDI_SelectPort *)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	try {
		sPort->getState(position);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position) {
	OPDI_SelectPort *sPort = (OPDI_SelectPort *)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (sPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	try {
		sPort->setPosition(position);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

#endif	//  OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

uint8_t opdi_get_dial_port_state(opdi_Port *port, int64_t *position) {
	OPDI_DialPort *dPort = (OPDI_DialPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	try {
		dPort->getState(position);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_dial_port_position(opdi_Port *port, int64_t position) {
	OPDI_DialPort *dPort = (OPDI_DialPort *)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (dPort->isReadonly())
		return OPDI_PORT_ACCESS_DENIED;

	try {
		dPort->setPosition(position);
	} catch (OPDI_Port::PortError &pe) {
		opdi_set_port_message(pe.message().c_str());
		return OPDI_PORT_ERROR;
	} catch (OPDI_Port::AccessDenied &ad) {
		opdi_set_port_message(ad.message().c_str());
		return OPDI_PORT_ACCESS_DENIED;
	}
	return OPDI_STATUS_OK;
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

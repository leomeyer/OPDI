
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
#include "Poco/UTF8String.h"
#include "Poco/FileStream.h"
#include "Poco/Process.h"
#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/NumberParser.h"
#include "Poco/RegularExpression.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"
#include "OPDI_Ports.h"
#include "OPDID_Ports.h"
#include "OPDID_TimerPort.h"
#include "OPDID_ExpressionPort.h"
#include "OPDID_ExecPort.h"

#define DEFAULT_IDLETIMEOUT_MS	180000
#define DEFAULT_TCP_PORT		13110

#define OPDID_CONFIG_FILE_SETTING	"__OPDID_CONFIG_FILE_PATH"

#define OPDID_SUPPORTED_PROTOCOLS	"EP,BP"

// global device flags
uint16_t opdi_device_flags = 0;

#define ENCRYPTION_METHOD_MAXLENGTH		16
char opdi_encryption_method[(ENCRYPTION_METHOD_MAXLENGTH + 1)];
#define ENCRYPTION_KEY_MAXLENGTH		16
char opdi_encryption_key[(ENCRYPTION_KEY_MAXLENGTH + 1)];

#define OPDI_ENCRYPTION_BLOCKSIZE	16
uint16_t opdi_encryption_blocksize = OPDI_ENCRYPTION_BLOCKSIZE;

#ifdef _MSC_VER
unsigned char opdi_encryption_buffer[OPDI_ENCRYPTION_BLOCKSIZE];
unsigned char opdi_encryption_buffer_2[OPDI_ENCRYPTION_BLOCKSIZE];
#endif

// slave protocol callback
void protocol_callback(uint8_t state) {
	Opdi->protocolCallback(state);
}

AbstractOPDID::AbstractOPDID(void) {
	this->majorVersion = OPDID_MAJOR_VERSION;
	this->minorVersion = OPDID_MINOR_VERSION;
	this->patchVersion = OPDID_PATCH_VERSION;

	this->logVerbosity = UNKNOWN;
	this->persistentConfig = NULL;

	this->logger = NULL;
	this->timestampFormat = "%Y-%m-%d %H:%M:%S.%i";

	this->monSecondPos = 0;
	this->totalMicroseconds = 0;
	this->targetFramesPerSecond = 200;
	this->waitingCallsPerSecond = 0;

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

uint8_t AbstractOPDID::idleTimeoutReached(void) {
	uint8_t result = OPDI::idleTimeoutReached();

	// log if idle timeout occurred
	if (result == OPDI_DISCONNECTED)
		this->logNormal("Idle timeout reached");

	return result;
}

void AbstractOPDID::protocolCallback(uint8_t protState) {
	if (protState == OPDI_PROTOCOL_START_HANDSHAKE) {
		this->logVerbose("Handshake started");
	} else
	if (protState == OPDI_PROTOCOL_CONNECTED) {
		this->connected();
	} else
	if (protState == OPDI_PROTOCOL_DISCONNECTED) {
		this->disconnected();
	}
}

void AbstractOPDID::connected() {
	this->logNormal("Connected to: " + this->masterName);

	// notify registered listeners
	ConnectionListenerList::iterator it = this->connectionListeners.begin();
	while (it != this->connectionListeners.end()) {
		(*it)->masterConnected();
		it++;
	}
}

void AbstractOPDID::disconnected() {
	this->logNormal("Disconnected from: " + this->masterName);

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

	this->logNormal("OPDID version " + this->to_string(this->majorVersion) + "." + this->to_string(this->minorVersion) + "." + this->to_string(this->patchVersion) + " (c) Leo Meyer 2015");
	this->logVerbose("Build: " + std::string(__DATE__) + " " + std::string(__TIME__));
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

void AbstractOPDID::println(std::string text) {
	this->println(text.c_str());
}

void AbstractOPDID::printlne(std::string text) {
	this->printlne(text.c_str());
}

void AbstractOPDID::log(std::string text) {
	// Important: log must be thread-safe.
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

	// Important: log must be thread-safe.
	Poco::Mutex::ScopedLock(this->mutex);

	std::string msg = "[" + this->getTimestampStr() + "] WARNING: " + text;
	if (this->logger != NULL) {
		this->logger->warning(msg);
	}
	this->printlne(msg);
}

void AbstractOPDID::logError(std::string text) {
	// Important: log must be thread-safe.
	Poco::Mutex::ScopedLock(this->mutex);

	std::string msg = "ERROR: " + text;
	if (this->logger != NULL) {
		this->logger->error(msg);
	}
	this->printlne("[" + this->getTimestampStr() + "] " + msg);
}

void AbstractOPDID::logNormal(std::string message) {
	if (this->logVerbosity >= AbstractOPDID::NORMAL) {
		this->log(message);
	}
}

void AbstractOPDID::logVerbose(std::string message) {
	if (this->logVerbosity >= AbstractOPDID::VERBOSE) {
		this->log(message);
	}
}

void AbstractOPDID::logDebug(std::string message) {
	if (this->logVerbosity >= AbstractOPDID::DEBUG) {
		this->log(message);
	}
}

void AbstractOPDID::logExtreme(std::string message) {
	if (this->logVerbosity >= AbstractOPDID::EXTREME) {
		this->log(message);
	}
}

int AbstractOPDID::startup(std::vector<std::string> args, std::map<std::string, std::string> environment) {
	this->environment = environment;
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> configuration = NULL;

	// add default environment parameters
	this->environment["$DATETIME"] = Poco::DateTimeFormatter::format(Poco::LocalDateTime(), this->timestampFormat);
	this->environment["$LOG_DATETIME"] = Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%Y%m%d_%H%M%S");

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
				configuration = this->readConfiguration(args.at(i), this->environment);
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
	if (configuration.isNull())
		throw Poco::SyntaxException("Expected argument: -c <config_file>");

	this->sayHello();

	// create view to "General" section
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> general = configuration->createView("General");
	this->setGeneralConfiguration(general);

	this->setupRoot(configuration);

	this->logVerbose("Node setup complete, preparing ports");

	this->sortPorts();
	this->preparePorts();

	// create view to "Connection" section
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> connection = configuration->createView("Connection");

	return this->setupConnection(connection);
}

void AbstractOPDID::lockResource(std::string resourceID, std::string lockerID) {
	this->logDebug("Trying to lock resource '" + resourceID + "' for " + lockerID);
	// try to locate the resource ID
	LockedResources::const_iterator it = this->lockedResources.find(resourceID);
	// resource already used?
	if (it != this->lockedResources.end())
		throw Poco::DataException("Resource requested by " + lockerID + " is already in use by " + it->second + ": " + resourceID);

	// store the resource
	this->lockedResources[resourceID] = lockerID;
}

AbstractOPDID::LogVerbosity AbstractOPDID::getConfigLogVerbosity(Poco::Util::AbstractConfiguration *config, LogVerbosity defaultVerbosity) {
	std::string logVerbosityStr = config->getString("LogVerbosity", "");

	if ((logVerbosityStr != "")) {
		if (logVerbosityStr == "Quiet") {
			return QUIET;
		} else
		if (logVerbosityStr == "Normal") {
			return NORMAL;
		} else
		if (logVerbosityStr == "Verbose") {
			return VERBOSE;
		} else
		if (logVerbosityStr == "Debug") {
			return DEBUG;
		} else
		if (logVerbosityStr == "Extreme") {
			return EXTREME;
		} else
			throw Poco::InvalidArgumentException("Verbosity level unknown (expected one of 'Quiet', 'Normal', 'Verbose', 'Debug' or 'Extreme')", logVerbosityStr);
	}
	return defaultVerbosity;
}

Poco::Util::AbstractConfiguration *AbstractOPDID::getConfigForState(Poco::Util::AbstractConfiguration *baseConfig, std::string viewName) {
	// replace configuration with a layered configuration that uses the persistent
	// configuration with higher priority
	// thus, port states will be pulled from the persistent configuration if they are present
	Poco::Util::LayeredConfiguration *newConfig = new Poco::Util::LayeredConfiguration();
	// persistent configuration specified?
	if (this->persistentConfig != NULL) {
		// persistent config has high priority
		if (viewName == "")
			newConfig->add(this->persistentConfig, 0);
		else {
			Poco::AutoPtr<Poco::Util::AbstractConfiguration> configView = this->persistentConfig->createView(viewName);
			newConfig->add(configView);
		}
	}
	// standard config has low priority
	newConfig->add(baseConfig, 10);

	return newConfig;
}

void AbstractOPDID::setGeneralConfiguration(Poco::Util::AbstractConfiguration *general) {
	this->logVerbose("Setting up general configuration");

	// initialize persistent configuration if specified
	std::string persistentFile = this->getConfigString(general, "PersistentConfig", "", false);
	if (persistentFile != "") {
		// determine application-relative path depending on location of config file
		std::string configFilePath = general->getString(OPDID_CONFIG_FILE_SETTING, "");
		if (configFilePath == "")
			throw Poco::DataException("Programming error: Configuration file path not specified in config settings");

		Poco::Path filePath(configFilePath);
		Poco::Path absPath(filePath.absolute());
		Poco::Path parentPath = absPath.parent();
		// append or replace new config file path to path of previous config file
		Poco::Path finalPath = parentPath.resolve(persistentFile);
		persistentFile = finalPath.toString();
		Poco::File file(finalPath);

		// the persistent configuration is not an INI file (because POCO can't write INI files)
		// but a "properties-format" file
		if (file.exists())
			// load existing file
			this->persistentConfig = new Poco::Util::PropertyFileConfiguration(persistentFile);
		else
			// file does not yet exist; will be created when persisting state
			this->persistentConfig = new Poco::Util::PropertyFileConfiguration();
		this->persistentConfigFile = persistentFile;
	}

	this->heartbeatFile = this->getConfigString(general, "HeartbeatFile", "", false);
	this->targetFramesPerSecond = general->getInt("TargetFPS", this->targetFramesPerSecond);

	std::string slaveName = this->getConfigString(general, "SlaveName", "", true);
	int messageTimeout = general->getInt("MessageTimeout", OPDI_DEFAULT_MESSAGE_TIMEOUT);
	if ((messageTimeout < 0) || (messageTimeout > 65535))
			throw Poco::InvalidArgumentException("MessageTimeout must be greater than 0 and may not exceed 65535", to_string(messageTimeout));
	opdi_set_timeout(messageTimeout);

	int idleTimeout = general->getInt("IdleTimeout", DEFAULT_IDLETIMEOUT_MS);

	this->deviceInfo = general->getString("DeviceInfo", "");

	// set log verbosity only if it's not already set
	if (this->logVerbosity == UNKNOWN) {
		this->logVerbosity = this->getConfigLogVerbosity(general, NORMAL);
	}

	this->allowHiddenPorts = general->getBool("AllowHidden", true);

	// encryption defined?
	std::string encryptionNode = general->getString("Encryption", "");
	if (encryptionNode != "") {
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> encryptionConfig = general->createView(encryptionNode);
		this->configureEncryption(encryptionConfig);
	}

	// authentication defined?
	std::string authenticationNode = general->getString("Authentication", "");
	if (authenticationNode != "") {
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> authenticationConfig = general->createView(authenticationNode);
		this->configureAuthentication(authenticationConfig);
	}

	// initialize OPDI slave
	this->setup(slaveName.c_str(), idleTimeout);
}

void AbstractOPDID::configureEncryption(Poco::Util::AbstractConfiguration *config) {
	this->logVerbose("Configuring encryption");

	std::string type = config->getString("Type", "");
	if (type == "AES") {
		std::string key = config->getString("Key", "");
		if (key.length() != 16)
			throw Poco::DataException("AES encryption setting 'Key' must be specified and 16 characters long");

		strncpy(opdi_encryption_method, "AES", ENCRYPTION_METHOD_MAXLENGTH);
		strncpy(opdi_encryption_key, key.c_str(), ENCRYPTION_KEY_MAXLENGTH);
	} else
		throw Poco::DataException("Encryption type not supported, expected 'AES': " + type);
}

void AbstractOPDID::configureAuthentication(Poco::Util::AbstractConfiguration *config) {
	this->logVerbose("Configuring authentication");

	std::string type = config->getString("Type", "");
	if (type == "Login") {
		std::string username = config->getString("Username", "");
		if (username == "")
			throw Poco::DataException("Authentication setting 'Username' must be specified");
		this->loginUser = username;

		this->loginPassword = config->getString("Password", "");

		// flag the device: expect authentication
		opdi_device_flags |= OPDI_FLAG_AUTHENTICATION_REQUIRED;
	} else
		throw Poco::DataException("Authentication type not supported, expected 'Login': " + type);
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
	this->logVerbose("Setting up group: " + group);

	OPDI_PortGroup *portGroup = new OPDI_PortGroup(group.c_str());
	this->configureGroup(groupConfig, portGroup, 0);

	this->addPortGroup(portGroup);
}

void AbstractOPDID::setupInclude(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig, std::string node) {
	this->logVerbose("Setting up include: " + node);

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
	this->logVerbose(node + ": Evaluating include parameters section: " + node + ".Parameters");

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> paramConfig = parentConfig->createView(node + ".Parameters");

	// get list of parameters
	Poco::Util::AbstractConfiguration::Keys paramKeys;
	paramConfig->keys("", paramKeys);

	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = paramKeys.begin(); it != paramKeys.end(); ++it) {
		std::string param = paramConfig->getString(*it, "");
		// store in the map
		parameters[*it] = param;
	}

	// warn if no parameters
	if ((parameters.size() == 0))
		this->logNormal(node + ": No parameters for include in section " + node + ".Parameters found, is this intended?");

	this->logVerbose(node + ": Processing include file: " + filename);

	Poco::Util::AbstractConfiguration *includeConfig = this->readConfiguration(filename, parameters);

	// setup the root node of the included configuration
	this->setupRoot(includeConfig);

	if (this->logVerbosity >= VERBOSE) {
		this->logVerbose(node + ": Include file " + filename + " processed successfully.");
		std::string configFilePath = parentConfig->getString(OPDID_CONFIG_FILE_SETTING, "");
		if (configFilePath != "")
			this->logVerbose("Continuing with parent configuration file: " + configFilePath);
	}
}

void AbstractOPDID::configurePort(Poco::Util::AbstractConfiguration *portConfig, OPDI_Port *port, int defaultFlags) {
	// ports can be hidden if allowed
	if (this->allowHiddenPorts)
		port->setHidden(portConfig->getBool("Hidden", false));

	// ports can be readonly
	port->setReadonly(portConfig->getBool("Readonly", false));

	// ports can be persistent
	port->setPersistent(portConfig->getBool("Persistent", false));

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

	port->tag = this->getConfigString(portConfig, "Tag", "", false);

	port->orderID = portConfig->getInt("OrderID", -1);
}

void AbstractOPDID::configureDigitalPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DigitalPort *port) {
	this->configurePort(portConfig, port, 0);

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> stateConfig = this->getConfigForState(portConfig, port->getID());

	std::string portMode = this->getConfigString(stateConfig, "Mode", "", false);
	if (portMode == "Input") {
		port->setMode(OPDI_DIGITAL_MODE_INPUT_FLOATING);
	} else if (portMode == "Input with pullup") {
		port->setMode(OPDI_DIGITAL_MODE_INPUT_PULLUP);
	} else if (portMode == "Input with pulldown") {
		port->setMode(OPDI_DIGITAL_MODE_INPUT_PULLDOWN);
	} else if (portMode == "Output") {
		port->setMode(OPDI_DIGITAL_MODE_OUTPUT);
	} else if (portMode != "")
		throw Poco::DataException("Unknown Mode specified; expected 'Input', 'Input with pullup', 'Input with pulldown', or 'Output'", portMode);

	std::string portLine = this->getConfigString(stateConfig, "Line", "", false);
	if (portLine == "High") {
		port->setLine(1);
	} else if (portLine == "Low") {
		port->setLine(0);
	} else if (portLine != "")
		throw Poco::DataException("Unknown Line specified; expected 'Low' or 'High'", portLine);
}

void AbstractOPDID::setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up emulated digital port: " + port);

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

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> stateConfig = this->getConfigForState(portConfig, port->getID());

	std::string mode = this->getConfigString(stateConfig, "Mode", "", false);
	if (mode == "Input")
		port->setMode(0);
	else if (mode == "Output")
		port->setMode(1);
	else if (mode != "")
		throw Poco::ApplicationException("Unknown mode specified; expected 'Input' or 'Output'", mode);

	// TODO reference?

	if (stateConfig->hasProperty("Resolution")) {
		uint8_t resolution = stateConfig->getInt("Resolution", OPDI_ANALOG_PORT_RESOLUTION_12);
		port->setResolution(resolution);
	}
	if (stateConfig->hasProperty("Value")) {
		uint32_t value = stateConfig->getInt("Value", 0);
		port->setValue(value);
	}
}

void AbstractOPDID::setupEmulatedAnalogPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up emulated analog port: " + port);

	OPDI_AnalogPort *anaPort = new OPDI_AnalogPort(port.c_str());
	this->configureAnalogPort(portConfig, anaPort);

	this->addPort(anaPort);
}

void AbstractOPDID::configureSelectPort(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig, OPDI_SelectPort *port) {
	this->configurePort(portConfig, port, 0);

	// the select port requires a prefix or section "<portID>.Items"
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> portItems = parentConfig->createView(std::string(port->getID()) + ".Items");

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

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> stateConfig = this->getConfigForState(portConfig, port->getID());

	if (stateConfig->getString("Position", "") != "") {
		uint16_t position = stateConfig->getInt("Position", 0);
		if ((position < 0) || (position >= charItems.size()))
			throw Poco::DataException("Wrong select port setting: Position is out of range: " + to_string(position));
		port->setPosition(position);
	}
}

void AbstractOPDID::setupEmulatedSelectPort(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig, std::string port) {
	this->logVerbose("Setting up emulated select port: " + port);

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

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> stateConfig = this->getConfigForState(portConfig, port->getID());

	int64_t position = stateConfig->getInt64("Position", min);
	if ((position < min) || (position > max))
		throw Poco::DataException("Wrong dial port setting: Position is out of range: " + to_string(position));

	port->setMin(min);
	port->setMax(max);
	port->setStep(step);
	port->setPosition(position);
}

void AbstractOPDID::setupEmulatedDialPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up emulated dial port: " + port);

	OPDI_DialPort *dialPort = new OPDI_DialPort(port.c_str());
	this->configureDialPort(portConfig, dialPort);

	this->addPort(dialPort);
}

void AbstractOPDID::configureStreamingPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_StreamingPort *port) {
	this->configurePort(portConfig, port, 0);
}

void AbstractOPDID::setupSerialStreamingPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up serial streaming port: " + port);

	OPDID_SerialStreamingPort *ssPort = new OPDID_SerialStreamingPort(this, port.c_str());
	ssPort->configure(portConfig);

	this->addPort(ssPort);
}

void AbstractOPDID::setupLoggerPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up Logger port: " + port);

	OPDID_LoggerPort *logPort = new OPDID_LoggerPort(this, port.c_str());
	logPort->configure(portConfig);

	this->addPort(logPort);
}

void AbstractOPDID::setupLogicPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up LogicPort: " + port);

	OPDID_LogicPort *dlPort = new OPDID_LogicPort(this, port.c_str());
	dlPort->configure(portConfig);

	this->addPort(dlPort);
}

void AbstractOPDID::setupFaderPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up FaderPort: " + port);

	OPDID_FaderPort *fPort = new OPDID_FaderPort(this, port.c_str());
	fPort->configure(portConfig);

	this->addPort(fPort);
}

void AbstractOPDID::setupExecPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up ExecPort: " + port);

	OPDID_ExecPort *ePort = new OPDID_ExecPort(this, port.c_str());
	ePort->configure(portConfig);

	this->addPort(ePort);
}

void AbstractOPDID::setupPulsePort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up PulsePort: " + port);

	OPDID_PulsePort *pulsePort = new OPDID_PulsePort(this, port.c_str());
	pulsePort->configure(portConfig);

	this->addPort(pulsePort);
}

void AbstractOPDID::setupSelectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up SelectorPort: " + port);

	OPDID_SelectorPort *selectorPort = new OPDID_SelectorPort(this, port.c_str());
	selectorPort->configure(portConfig);

	this->addPort(selectorPort);
}

#ifdef OPDID_USE_EXPRTK
void AbstractOPDID::setupExpressionPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up Expression: " + port);

	OPDID_ExpressionPort *expressionPort = new OPDID_ExpressionPort(this, port.c_str());
	expressionPort->configure(portConfig);

	this->addPort(expressionPort);
}
#endif	// def OPDID_USE_EXPRTK

void AbstractOPDID::setupTimerPort(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig, std::string port) {
	this->logVerbose("Setting up Timer: " + port);

	OPDID_TimerPort *timerPort = new OPDID_TimerPort(this, port.c_str());
	timerPort->configure(portConfig, parentConfig);

	this->addPort(timerPort);
}

void AbstractOPDID::setupErrorDetectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->logVerbose("Setting up ErrorDetector: " + port);

	OPDID_ErrorDetectorPort *edPort = new OPDID_ErrorDetectorPort(this, port.c_str());
	edPort->configure(portConfig);

	this->addPort(edPort);
}

void AbstractOPDID::setupNode(Poco::Util::AbstractConfiguration *config, std::string node) {
	this->logVerbose("Setting up node: " + node);

	// emit warnings if node IDs deviate from best practices

	// check whether node name is all numeric
	double value = 0;
	if (Poco::NumberParser::tryParseFloat(node, value)) {
		this->logWarning("Node ID can be confused with a number: " + node);
	}
	// check whether there are special characters
	Poco::RegularExpression re(".*\\W.*");
	if (re.match(node)) {
		this->logWarning("Node ID should not contain special characters: " + node);
	}

	// create node section view
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = config->createView(node);

	// get node information
	std::string nodeDriver = this->getConfigString(nodeConfig, "Driver", "", false);

	// driver specified?
	if (nodeDriver != "") {
		this->logVerbose("Loading plugin driver: " + nodeDriver);

		// try to load the plugin; the driver name is the (platform dependent) library file name
		IOPDIDPlugin *plugin = this->getPlugin(nodeDriver);

		// add plugin to internal list (avoids memory leaks)
		this->pluginList.push_back(plugin);

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
		if (nodeType == "Logger") {
			this->setupLoggerPort(nodeConfig, node);
		} else
		if (nodeType == "Logic") {
			this->setupLogicPort(nodeConfig, node);
		} else
		if (nodeType == "Pulse") {
			this->setupPulsePort(nodeConfig, node);
		} else
		if (nodeType == "Selector") {
			this->setupSelectorPort(nodeConfig, node);
#ifdef OPDID_USE_EXPRTK
		} else
		if (nodeType == "Expression") {
			this->setupExpressionPort(nodeConfig, node);
#else
#pragma message( "Expression library not included, cannot use the Expression node type" )
#endif	// def OPDID_USE_EXPRTK
		} else
		if (nodeType == "Timer") {
			this->setupTimerPort(nodeConfig, config, node);
		} else
		if (nodeType == "ErrorDetector") {
			this->setupErrorDetectorPort(nodeConfig, node);
		} else
		if (nodeType == "Fader") {
			this->setupFaderPort(nodeConfig, node);
		} else
		if (nodeType == "Exec") {
			this->setupExecPort(nodeConfig, node);
		} else
			throw Poco::DataException("Invalid configuration: Unknown node type", nodeType);
	}
}

void AbstractOPDID::setupRoot(Poco::Util::AbstractConfiguration *config) {
	// enumerate section "Root"
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodes = config->createView("Root");
	this->logVerbose("Setting up root nodes");

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
		this->logNormal(filename + ": No nodes in section Root found, is this intended?");
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
	this->logVerbose(std::string("Setting up connection for slave: ") + this->slaveName);
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
		this->logNormal("Warning: Could not parse build date and time to check possible ABI violation of driver " + driver + ": " + __DATE__ + " " + __TIME__);
	} else {
		// check whether the plugin driver file is more recent
		Poco::File driverFile(driver);
		// convert build time to UTC
		buildTime.makeUTC(Poco::Timezone::tzd());
		// assume both files have been built in the same time zone (getLastModified also returns UTC)
		if ((driverFile.getLastModified() < buildTime.timestamp()))
			this->logNormal("Warning: Plugin module " + driver + " is older than the main binary; possible ABI conflict! In case of strange errors please recompile the plugin!");
	}
}

uint8_t AbstractOPDID::waiting(uint8_t canSend) {
	uint8_t result;

	// add up microseconds of idle time
	this->totalMicroseconds += this->idleStopwatch.elapsed();
	this->waitingCallsPerSecond++;

	// start local stopwatch
	Poco::Stopwatch stopwatch;
	stopwatch.start();

	// exception-safe processing
	try {
		result = OPDI::waiting(canSend);
	} catch (Poco::Exception &pe) {
		this->logError(std::string("Unhandled exception while housekeeping: ") + pe.message());
	} catch (std::exception &e) {
		this->logError(std::string("Unhandled exception while housekeeping: ") + e.what());
	} catch (...) {
		this->logError(std::string("Unknown error while housekeeping"));
	}
	// TODO decide: ignore errors or abort?

	if (result != OPDI_STATUS_OK)
		return result;

	// add runtime statistics to monitor buffer
	this->monSecondStats[this->monSecondPos] = stopwatch.elapsed();		// microseconds
	// add up microseconds of processing time
	this->totalMicroseconds += this->monSecondStats[this->monSecondPos];
	this->monSecondPos++;
	// buffer rollover?
	if (this->monSecondPos >= maxSecondStats) {
		this->monSecondPos = 0;
	}
	// collect statistics until a second has elapsed
	if (this->totalMicroseconds >= 1000000) {
		uint64_t maxProcTime = -1;
		uint64_t sumProcTime = 0;
		for (int i = 0; i < this->monSecondPos; i++) {
			sumProcTime += this->monSecondStats[i];
			if (this->monSecondStats[i] > maxProcTime)
				maxProcTime = this->monSecondStats[i];
		}
		this->monSecondPos = 0;
		this->framesPerSecond = this->waitingCallsPerSecond * 1000000.0 / this->totalMicroseconds;
		double procAverageUsPerCall = (double)sumProcTime / this->waitingCallsPerSecond;	// microseconds
		double load = sumProcTime * 1.0 / this->totalMicroseconds * 100.0;

		// ignore first calculation results
		if (this->framesPerSecond > 0) {
			if (this->logVerbosity >= EXTREME) {
				this->logExtreme("Elapsed processing time: " + this->to_string(this->totalMicroseconds) + " us");
				this->logExtreme("Loop iterations per second: " + this->to_string(this->framesPerSecond));
				this->logExtreme("Processing time average per iteration: " + this->to_string(procAverageUsPerCall) + " us");
				this->logExtreme("Processing load: " + this->to_string(load) + "%");
			}
			if (load > 90.0)
				this->logWarning("Processing the doWork loop takes very long; load = " + this->to_string(load) + "%");
		}

		// reset counters
		this->totalMicroseconds = 0;
		this->waitingCallsPerSecond = 0;

		// write status file if specified
		if (this->heartbeatFile != "") {
			this->logExtreme("Writing heartbeat file: " + this->heartbeatFile);
			std::string output = this->getTimestampStr() + ": pid=" + this->to_string(Poco::Process::id()) + "; fps=" + this->to_string(this->framesPerSecond) + "; load=" + this->to_string(load) + "%";
			Poco::FileOutputStream fos(this->heartbeatFile);
			fos.write(output.c_str(), output.length());
			fos.close();
		}
	}

	// restart idle stopwatch to measure time until waiting() is called again
	this->idleStopwatch.restart();

	return result;
}

bool icompare_pred(unsigned char a, unsigned char b)
{
    return std::tolower(a) == std::tolower(b);
}

bool icompare(std::string const& a, std::string const& b)
{
    return std::lexicographical_compare(a.begin(), a.end(),
                                        b.begin(), b.end(), icompare_pred);
}

uint8_t AbstractOPDID::setPassword(std::string password) {
	// TODO fix: username/password side channel leak (timing attack) (?)
	// username must match case insensitive
	if (Poco::UTF8::icompare(this->loginUser, this->username))
		return OPDI_AUTHENTICATION_FAILED;
	// password must match case sensitive
	if (this->loginPassword != password)
		return OPDI_AUTHENTICATION_FAILED;

	return OPDI_STATUS_OK;
}

std::string AbstractOPDID::getExtendedDeviceInfo(void) {
	return this->deviceInfo;
}

uint8_t AbstractOPDID::refresh(OPDI_Port **ports) {
	uint8_t result = OPDI::refresh(ports);
	if (result != OPDI_STATUS_OK)
		return result;

	if (this->logVerbosity >= VERBOSE) {
		OPDI_Port *port = ports[0];
		uint8_t i = 0;
		while (port != NULL) {
			this->logVerbose("Sent refresh for port: " + port->ID());
			port = ports[++i];
		}
	}

	return result;
}

void AbstractOPDID::persist(OPDI_Port *port) {
	if (this->persistentConfig == NULL) {
		this->logWarning(std::string("Unable to persist state for port ") + port->getID() + ": No configuration file specified; use 'PersistentConfig' in the General configuration section");
		return;
	}

	this->logDebug("Trying to persist port state for: " + port->ID());

	try {
		// evaluation depends on port type
		if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
			uint8_t mode;
			uint8_t line;
			((OPDI_DigitalPort *)port)->getState(&mode, &line);
			std::string modeStr = "";
			if (mode == OPDI_DIGITAL_MODE_INPUT_FLOATING)
				modeStr = "Input";
			else if (mode == OPDI_DIGITAL_MODE_INPUT_PULLUP)
				modeStr = "Input with pullup";
			else if (mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN)
				modeStr = "Input with pulldown";
			else if (mode == OPDI_DIGITAL_MODE_OUTPUT)
				modeStr = "Output";
			std::string lineStr = "";
			if (line == 0)
				lineStr = "Low";
			else if (line == 1)
				lineStr = "High";

			if (modeStr != "") {
				this->logDebug("Writing port state for: " + port->ID() + "; mode = " + modeStr);
				this->persistentConfig->setString(port->ID() + ".Mode", modeStr);
			}
			if (lineStr != "") {
				this->logDebug("Writing port state for: " + port->ID() + "; line = " + lineStr);
				this->persistentConfig->setString(port->ID() + ".Line", lineStr);
			}
		} else
		if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
			uint8_t mode;
			uint8_t resolution;
			uint8_t reference;
			int32_t value;
			((OPDI_AnalogPort *)port)->getState(&mode, &resolution, &reference, &value);
			std::string modeStr = "";
			if (mode == OPDI_ANALOG_MODE_INPUT)
				modeStr = "Input";
			else if (mode == OPDI_ANALOG_MODE_OUTPUT)
				modeStr = "Output";
			// TODO reference
			std::string refStr = "";

			if (modeStr != "") {
				this->logDebug("Writing port state for: " + port->ID() + "; mode = " + modeStr);
				this->persistentConfig->setString(port->ID() + ".Mode", modeStr);
			}
			this->logDebug("Writing port state for: " + port->ID() + "; resolution = " + this->to_string(resolution));
			this->persistentConfig->setInt(port->ID() + ".Resolution", resolution);
			this->logDebug("Writing port state for: " + port->ID() + "; value = " + this->to_string(value));
			this->persistentConfig->setInt(port->ID() + ".Value", value);
		} else
		if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
			int64_t position;
			((OPDI_DialPort *)port)->getState(&position);

			this->logDebug("Writing port state for: " + port->ID() + "; position = " + this->to_string(position));
			this->persistentConfig->setInt64(port->ID() + ".Position", position);
		} else
		if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
			uint16_t position;
			((OPDI_SelectPort *)port)->getState(&position);

			this->logDebug("Writing port state for: " + port->ID() + "; position = " + this->to_string(position));
			this->persistentConfig->setInt(port->ID() + ".Position", position);
		} else {
			this->logDebug("Unable to persist port state for: " + port->ID() + "; unknown port type: " + port->getType());
			return;
		}
	} catch (Poco::Exception &e) {
		this->logWarning("Unable to persist state of port " + port->ID() + ": " + e.message());
		return;
	}
	this->persistentConfig->setString("LastChange", this->getTimestampStr());
	this->persistentConfig->save(this->persistentConfigFile);
}

std::string AbstractOPDID::getPortStateStr(OPDI_Port* port) {
	try {
		if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
			uint8_t line;
			uint8_t mode;
			((OPDI_DigitalPort*)port)->getState(&mode, &line);
			char c[] = " ";
			c[0] = line + '0';
			return std::string(c);
		}
		if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
			double value = ((OPDI_AnalogPort *)port)->getRelativeValue();
			return this->to_string(value);
		}
		if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
			uint16_t position;
			((OPDI_SelectPort *)port)->getState(&position);
			return this->to_string(position);
		}
		if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
			int64_t position;
			((OPDI_DialPort *)port)->getState(&position);
			return this->to_string(position);
		}
		// unknown port type
		return "";
	} catch (...) {
		// in case of error return an empty string
		return "";
	}
}

double AbstractOPDID::getPortValue(OPDI_Port* port) {
	double value = 0;

	// evaluation depends on port type
	if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
		// digital port: Low = 0; High = 1
		uint8_t mode;
		uint8_t line;
		((OPDI_DigitalPort *)port)->getState(&mode, &line);
		value = line;
	} else
	if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
		// analog port: relative value (0..1)
		value = ((OPDI_AnalogPort *)port)->getRelativeValue();
	} else
	if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
		// dial port: absolute value
		int64_t position;
		((OPDI_DialPort *)port)->getState(&position);
		value = (double)position;
	} else
	if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
		// select port: current position number
		uint16_t position;
		((OPDI_SelectPort *)port)->getState(&position);
		value = position;
	} else
		// port type not supported
		throw Poco::Exception("Port type not supported");

	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following functions implement the glue code between C and C++.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t opdi_slave_callback(OPDIFunctionCode opdiFunctionCode, char *buffer, size_t data) {

	switch (opdiFunctionCode) {
	case OPDI_FUNCTION_GET_CONFIG_NAME: 
		strncpy(buffer, Opdi->getSlaveName().c_str(), data); 
		return OPDI_STATUS_OK;
	case OPDI_FUNCTION_SET_MASTER_NAME: 
		return Opdi->setMasterName(buffer);
	case OPDI_FUNCTION_GET_SUPPORTED_PROTOCOLS: 
		strncpy(buffer, OPDID_SUPPORTED_PROTOCOLS, data); 
		return OPDI_STATUS_OK;
	case OPDI_FUNCTION_GET_ENCODING: 
		strncpy(buffer, Opdi->getEncoding().c_str(), data); 
		return OPDI_STATUS_OK;
	case OPDI_FUNCTION_SET_LANGUAGES: 
		return Opdi->setLanguages(buffer);
	case OPDI_FUNCTION_GET_EXTENDED_DEVICEINFO:
		strncpy(buffer, Opdi->getExtendedDeviceInfo().c_str(), data); 
		return OPDI_STATUS_OK;
	case OPDI_FUNCTION_GET_EXTENDED_PORTINFO: {
		uint8_t code;
		std::string exPortInfo = Opdi->getExtendedPortInfo(buffer, &code);
		if (code != OPDI_STATUS_OK)
			return code;
		strncpy(buffer, exPortInfo.c_str(), data);
		return OPDI_STATUS_OK;
	}
	case OPDI_FUNCTION_GET_EXTENDED_PORTSTATE: {
		uint8_t code;
		std::string exPortInfo = Opdi->getExtendedPortState(buffer, &code);
		if (code != OPDI_STATUS_OK)
			return code;
		strncpy(buffer, exPortInfo.c_str(), data);
		return OPDI_STATUS_OK;
	}
#ifndef OPDI_NO_AUTHENTICATION
	case OPDI_FUNCTION_SET_USERNAME: return Opdi->setUsername(buffer);
	case OPDI_FUNCTION_SET_PASSWORD: return Opdi->setPassword(buffer);
#endif
	default: return OPDI_FUNCTION_UNKNOWN;
	}
}

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
	Opdi->logDebug(dirChar + message);
	return OPDI_STATUS_OK;
}

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

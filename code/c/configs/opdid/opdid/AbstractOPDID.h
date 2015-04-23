#pragma once

#include <sstream>
#include <list>

#include "Poco/Mutex.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/Util/IniFileConfiguration.h"

#include "OPDI.h"

class AbstractOPDID;

#define OPDID_MAJOR_VERSION		0
#define OPDID_MINOR_VERSION		1
#define OPDID_PATCH_VERSION		0

// protocol callback function for the OPDI slave implementation
extern void protocol_callback(uint8_t state);

/** The abstract plugin interface. */
struct IOPDIDPlugin {
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string nodeName, Poco::Util::AbstractConfiguration *nodeConfig) = 0;
};

/** The listener interface for plugin registrations. */
struct IOPDIDConnectionListener {
	/** Is called when a master successfully completed the handshake. */
	virtual void masterConnected(void) = 0;

	/** Is called when a master has disconnected. */
	virtual void masterDisconnected(void) = 0;
};

/** The abstract base class for OPDID implementations. */
class AbstractOPDID: public OPDI
{
protected:

	int majorVersion;
	int minorVersion;
	int patchVersion;

	bool allowHiddenPorts;

	Poco::Util::AbstractConfiguration *configuration;

	std::string masterName;

	Poco::Mutex mutex;

	typedef std::list<IOPDIDConnectionListener *> ConnectionListenerList;
	ConnectionListenerList connectionListeners;

	typedef std::map<uint8_t, std::string> OPDICodeTexts;
	OPDICodeTexts opdiCodeTexts;

	typedef std::map<std::string, std::string> LockedResources;
	LockedResources lockedResources;

	virtual void readConfiguration(std::string fileName);

public:

	enum LogVerbosity {
		UNKNOWN,
		QUIET,
		NORMAL,
		VERBOSE,
		DEBUG,
		EXTREME
	};

	int logVerbosity;

	AbstractOPDID(void);

	virtual ~AbstractOPDID(void);

	virtual void protocolCallback(uint8_t protState);

	virtual void connected(const char *masterName);

	virtual void disconnected(void);

	virtual void addConnectionListener(IOPDIDConnectionListener *listener);

	virtual void removeConnectionListener(IOPDIDConnectionListener *listener);

	/** Outputs a hello message. */
	virtual void sayHello(void);

	virtual void showHelp(void);

	virtual std::string getTimestampStr(void);

	virtual std::string getOPDIResult(uint8_t code);

	/** Returns the key's value from the configuration or the default value, if it is missing. If missing and isRequired is true, throws an exception. */
	virtual std::string getConfigString(Poco::Util::AbstractConfiguration *config, const std::string &key, const std::string &defaultValue, const bool isRequired);

	/** Outputs the specified text to an implementation-dependent output with an appended line break. */
	virtual void println(const char *text) = 0;

	/** Outputs the specified text to an implementation-dependent output with an appended line break. */
	virtual void println(std::string text);

	/** Converts a given object to a string. */
	template <class T> inline std::string to_string(const T& t);

	/** Writes a log message with a timestamp. */
	virtual void log(std::string text);

	virtual Poco::Util::AbstractConfiguration *getConfiguration(void);

	/** Starts processing the supplied arguments. */
	virtual int startup(std::vector<std::string>);
	
	/** Attempts to lock the resource with the specified ID. The resource can be anything, a pin number, a file name or whatever.
	 *  When the same resource is attempted to be locked twice this method throws an exception.
	 *  Use this mechanism to avoid resource conflicts. */
	virtual void lockResource(std::string resourceID, std::string lockerID);

	virtual void setGeneralConfiguration(Poco::Util::AbstractConfiguration *general);

	/** Reads common properties from the configuration and configures the port group. */
	virtual void configureGroup(Poco::Util::AbstractConfiguration *groupConfig, OPDI_PortGroup *group, int defaultFlags);

	virtual void setupGroup(Poco::Util::AbstractConfiguration *groupConfig, std::string group);

	/** Reads common properties from the configuration and configures the port. */
	virtual void configurePort(Poco::Util::AbstractConfiguration *portConfig, OPDI_Port *port, int defaultFlags);

	/** Reads special properties from the configuration and configures the digital port. */
	virtual void configureDigitalPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DigitalPort *port);

	virtual void setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	/** Reads special properties from the configuration and configures the analog port. */
	virtual void configureAnalogPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_AnalogPort *port);

	virtual void setupEmulatedAnalogPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	/** Reads special properties from the configuration and configures the select port. */
	virtual void configureSelectPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_SelectPort *port);

	virtual void setupEmulatedSelectPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	/** Reads special properties from the configuration and configures the dial port. */
	virtual void configureDialPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DialPort *port);

	virtual void setupEmulatedDialPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupLogicPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupPulsePort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupSelectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

#ifdef OPDI_USE_EXPRTK
	virtual void setupExpressionPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);
#endif	// def OPDI_USE_EXPRTK

	virtual void setupTimerPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupErrorDetectorPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	/** Configures the specified node. */
	virtual void setupNode(Poco::Util::AbstractConfiguration *config, std::string node);

	/** Starts enumerating the nodes of the Root section and configures the nodes. */
	virtual void setupRoot(Poco::Util::AbstractConfiguration *config);

	/** Sets up the connection from the specified configuration. */
	virtual int setupConnection(Poco::Util::AbstractConfiguration *config);

	/** Sets up a TCP listener and listens for incoming requests. This method does not return unless the program should exit. */
	virtual int setupTCP(std::string interface_, int port) = 0;

	/** Checks whether the supplied file is more recent than the current binary and logs a warning if yes. */
	virtual void warnIfPluginMoreRecent(std::string driver);

	/** Returns a pointer to the plugin object instance specified by the given driver. */
	virtual IOPDIDPlugin *getPlugin(std::string driver) = 0;

	virtual uint8_t waiting(uint8_t canSend) override;

};


template <class T> inline std::string AbstractOPDID::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

/** Define external singleton instance */
extern AbstractOPDID *Opdi;

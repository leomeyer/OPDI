#pragma once

#include <sstream>
#include <list>

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

	Poco::Util::AbstractConfiguration *configuration;

	virtual void readConfiguration(std::string fileName);

	std::string masterName;

	typedef std::list<IOPDIDConnectionListener *> ConnectionListenerList;
	ConnectionListenerList connectionListeners;

public:

	enum LogVerbosity {
		QUIET,
		NORMAL,
		VERBOSE
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

	virtual std::string getTimestampStr();

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

	virtual void setGeneralConfiguration(Poco::Util::AbstractConfiguration *general);

	virtual void configureDigitalPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_DigitalPort *port);

	virtual void setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void configureAnalogPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_AnalogPort *port);

	virtual void setupEmulatedAnalogPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void configureSelectPort(Poco::Util::AbstractConfiguration *portConfig, OPDI_SelectPort *port);

	virtual void setupEmulatedSelectPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupNode(Poco::Util::AbstractConfiguration *config, std::string node);

	virtual void setupNodes(Poco::Util::AbstractConfiguration *config);

	virtual int setupConnection(Poco::Util::AbstractConfiguration *config);

	virtual int setupTCP(std::string interface_, int port) = 0;

	virtual void warnIfPluginMoreRecent(std::string driver);

	virtual IOPDIDPlugin *getPlugin(std::string driver) = 0;
};


template <class T> inline std::string AbstractOPDID::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

/** Define external singleton instance */
extern AbstractOPDID *Opdi;
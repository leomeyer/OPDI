
#pragma once

#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/Util/IniFileConfiguration.h"

#include "OPDI.h"

class AbstractOPDID: public OPDI
{
protected:
	Poco::Util::AbstractConfiguration *configuration;

	virtual void readConfiguration(std::string);

public:
	AbstractOPDID(void);

	virtual ~AbstractOPDID(void);

	virtual std::string getConfigString(Poco::Util::AbstractConfiguration *config, const std::string &key, const std::string &defaultValue, const bool isRequired);

	/** Outputs the specified text to an implementation-dependent output. */
	virtual void print(const char *text) = 0;

	/** Outputs the specified text to an implementation-dependent output with an appended line break. */
	virtual void println(const char *text) = 0;

	template <class T> inline std::string to_string (const T& t);

	virtual void printlni(int i);

	/** Outputs a hello message. */
	virtual void sayHello(void);

	/** Starts processing the supplied arguments. */
	virtual int startup(std::vector<std::string>);

	virtual void setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port);

	virtual void setupPort(Poco::Util::AbstractConfiguration *config, std::string port);

	virtual void setupPorts(Poco::Util::AbstractConfiguration *config);

	virtual int setupConnection(Poco::Util::AbstractConfiguration *config);

	virtual int setupTCP(std::string interface_, int port) = 0;
};

/** Define external singleton instance */
extern AbstractOPDID *Opdi;
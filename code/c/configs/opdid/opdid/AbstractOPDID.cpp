
#include <string>
#include <vector>
#include <sstream>

#include "Poco/Exception.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"
#include "EmulatedPorts.h"

#define DEFAULT_IDLETIMEOUT_MS	180000
#define DEFAULT_TCP_PORT		13110

AbstractOPDID::AbstractOPDID(void)
{
	this->configuration = NULL;
}

AbstractOPDID::~AbstractOPDID(void)
{
}

void AbstractOPDID::sayHello(void) {
	std::cout << "OPDID version 0.1 (c) Leo Meyer 2015" << std::endl;
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

template <class T> inline std::string AbstractOPDID::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

void AbstractOPDID::printlni(int i) {
	std::stringstream str;
	str << i;
	this->println(str.str().c_str());
}

int AbstractOPDID::startup(std::vector<std::string> args) {
	// evaluate arguments
	for (int i = 0; i < args.size(); i++) {
		if (args.at(i) == "-c") {
			i++;
			if (args.size() == i) {
				throw Poco::SyntaxException("Expected configuration file name after argument -c");
			} else {
				this->print("Using configuration file: ");
				this->println(args.at(i).c_str());
				this->readConfiguration(args.at(i));
			}
		}
	}

	// no configuration?
	if (this->configuration == NULL)
		throw Poco::SyntaxException("Expected argument: -c <config_file_path>");

	// create view to "General" section
	Poco::Util::AbstractConfiguration *general = this->configuration->createView("General");
	this->println("Setting up general configuration");

	std::string slaveName = this->getConfigString(general, "SlaveName", "", true);
	int timeout = general->getInt("IdleTimeout", DEFAULT_IDLETIMEOUT_MS);

	this->setup(slaveName.c_str(), timeout);

	this->setupPorts(this->configuration);

	// create view to "Connection" section
	Poco::Util::AbstractConfiguration *connection = this->configuration->createView("Connection");

	return this->setupConnection(connection);
}

void AbstractOPDID::setupEmulatedDigitalPort(Poco::Util::AbstractConfiguration *portConfig, std::string port) {
	this->print("Setting up emulated digital port: ");
	this->println(port.c_str());

	std::string portLabel = this->getConfigString(portConfig, "Label", "", true);
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
	
	uint8_t flags = portConfig->getInt("Flags", 0);

	OPDI_EmulatedDigitalPort *digPort = new OPDI_EmulatedDigitalPort(port.c_str(), portLabel.c_str(), dircaps, flags);

	this->addPort(digPort);
}

void AbstractOPDID::setupPort(Poco::Util::AbstractConfiguration *config, std::string port) {
	this->print("Setting up port: ");
	this->println(port.c_str());

	// create port section view
	Poco::Util::AbstractConfiguration *portConfig = this->configuration->createView(port);

	// get port information
	std::string portType = this->getConfigString(portConfig, "Type", "", true);
	std::string portDriver = this->getConfigString(portConfig, "Driver", "", true);

	if (portType == "Digital") {
		if (portDriver == "EmulatedDigitalPort") {
			this->setupEmulatedDigitalPort(portConfig, port);
		} else
			throw Poco::DataException("Invalid configuration: Unknown digital port driver", portDriver);
	}
	else
		throw Poco::DataException("Invalid configuration: Unknown port type", portType);
}

void AbstractOPDID::setupPorts(Poco::Util::AbstractConfiguration *config) {
	// enumerate section "Ports"
	Poco::Util::AbstractConfiguration *ports = this->configuration->createView("Ports");
	this->println("Setting up ports");

	Poco::Util::AbstractConfiguration::Keys portKeys;
	ports->keys("", portKeys);
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = portKeys.begin(); it != portKeys.end(); ++it) {
		// check whether the port is active
		if (!ports->getBool(*it, false))
			continue;

		// check whether the port has already been added
		if (this->findPortByID(it->c_str()) == NULL) {
			this->setupPort(config, *it);
		} else 
			throw Poco::DataException("Invalid configuration: Duplicate port ID", *it);
	}
}

int AbstractOPDID::setupConnection(Poco::Util::AbstractConfiguration *config) {

	this->println("Setting up connection");
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
	if (direction == OPDI_DIR_INCOMING)
		Opdi->print(">");
	else
	if (direction == OPDI_DIR_OUTGOING)
		Opdi->print("<");
	else
	if (direction == OPDI_DIR_INCOMING_ENCR)
		Opdi->print("}");
	else
	if (direction == OPDI_DIR_OUTGOING_ENCR)
		Opdi->print("{");
	else
		Opdi->print("-");
	Opdi->println((const char *)message);
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

#endif

uint8_t opdi_choose_language(const char *languages) {
	// supports German?
	if (strcmp("de_DE", languages) == 0) {
		// TODO
	}

	return OPDI_STATUS_OK;
}

#ifdef OPDI_HAS_MESSAGE_HANDLED

uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	return Opdi->messageHandled(channel, parts);
}

#endif

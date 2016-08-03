//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-2014 Leo Meyer (leo@leomeyer.de)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

/** OPDI C++ wrapper implementation
 */

#include "OPDI.h"

#include <cstdlib>
#include <algorithm>    // std::sort
#include <string.h>

#include "opdi_constants.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "opdi_configspecs.h"
#include "opdi_platformfuncs.h"

namespace opdi {

//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t OPDI::shutdownInternal(void) {
	// shutdown and free all ports
	auto it = this->ports.begin();
	auto ite = this->ports.end();
	while (it != ite) {
		// ignore any errors during this process
		try {
			(*it)->shutdown();
			delete *it;
		}
		catch (...) {}
		++it;
	}
	this->ports.clear();
	this->disconnect();
	return OPDI_SHUTDOWN;
}

uint8_t OPDI::setup(const char* slaveName, int idleTimeout) {
	this->shutdownRequested = false;

	// initialize port list
	this->ports.clear();
	this->groups.clear();
	//this->first_portGroup = nullptr;
	//this->last_portGroup = nullptr;

	// copy slave name to internal buffer
	this->slaveName = slaveName;
	// set standard encoding to "utf-8"
	this->encoding = "utf-8";

	this->idle_timeout_ms = idleTimeout;

	return opdi_slave_init();
}

void OPDI::setIdleTimeout(uint32_t idleTimeoutMs) {
	this->idle_timeout_ms = idleTimeoutMs;
}

void OPDI::setEncoding(const std::string& encoding) {
	this->encoding = encoding;
}

std::string OPDI::getSlaveName(void) {
	return this->slaveName;
}

uint8_t OPDI::setMasterName(const std::string& masterName) {
	this->masterName = masterName;
	return OPDI_STATUS_OK;
}

std::string OPDI::getEncoding(void) {
	return this->encoding;
}

uint8_t OPDI::setLanguages(const std::string& languages) {
	this->languages = languages;
	return OPDI_STATUS_OK;
}

uint8_t OPDI::setUsername(const std::string& userName) {
	this->username = userName;
	return OPDI_STATUS_OK;
}

uint8_t OPDI::setPassword(const std::string& /*password*/) {
	return OPDI_STATUS_OK;
}

std::string OPDI::getExtendedDeviceInfo(void) {
	return "";
}

std::string OPDI::getExtendedPortInfo(char* portID, uint8_t* code) {
	opdi::Port* port = this->findPortByID(portID, false);
	if (port == nullptr) {
		*code = OPDI_PORT_UNKNOWN;
		return "";
	} else {
		*code = OPDI_STATUS_OK;
		return port->getExtendedInfo();
	}
}

std::string OPDI::getExtendedPortState(char* portID, uint8_t* code) {
	opdi::Port* port = this->findPortByID(portID, false);
	if (port == nullptr) {
		*code = OPDI_PORT_UNKNOWN;
		return "";
	} else {
		*code = OPDI_STATUS_OK;
		return port->getExtendedState();
	}
}

void OPDI::addPort(opdi::Port* port) {
	// associate port with this instance
	port->opdi = this;

	// first port?
	if (this->ports.size() == 0)
		this->currentOrderID = 0;

	this->ports.push_back(port);

	this->updatePortData(port);

	// do not use hidden ports for display sort order
	if (!port->isHidden()) {
		// order not defined?
		if (port->orderID < 0) {
			port->orderID = this->currentOrderID;
			this->currentOrderID++;
		} else
			if (this->currentOrderID < port->orderID)
				this->currentOrderID = port->orderID + 1;
	}
}

// possible race conditions here, if one thread updates port data while the other retrieves it
// - generally not a problem because slaves are usually single-threaded
void OPDI::updatePortData(opdi::Port* port) {
	// allocate port data structure if necessary
	opdi_Port* oPort = (opdi_Port*)port->data;
	if (oPort == nullptr) {
		oPort = (opdi_Port*)malloc(sizeof(opdi_Port));
		port->data = oPort;
		oPort->info.i = 0;
		oPort->info.ptr = nullptr;
		oPort->next = nullptr;
	}
	// update data
	oPort->id = (const char*)port->id;
	oPort->name = (const char*)port->label;
	oPort->type = (const char*)port->type;
	oPort->caps = (const char*)port->caps;
	oPort->flags = port->flags;

	// more complex ports require the pointer to contain additional information

	// check port type
	if (strcmp(port->type, OPDI_PORTTYPE_SELECT) == 0) {
		oPort->info.ptr = static_cast<opdi::SelectPort*>(port)->items;
	} else
	if (strcmp(port->type, OPDI_PORTTYPE_DIAL) == 0) {
		// release additional data structure memory
		if (oPort->info.ptr != nullptr)
			free(oPort->info.ptr);
		// allocate additional data structure memory
		struct opdi_DialPortInfo* dialPortInfo = (struct opdi_DialPortInfo*)malloc(sizeof(opdi_DialPortInfo));
		oPort->info.ptr = dialPortInfo;
		dialPortInfo->min = static_cast<opdi::DialPort*>(port)->minValue;
		dialPortInfo->max = static_cast<opdi::DialPort*>(port)->maxValue;
		dialPortInfo->step = static_cast<opdi::DialPort*>(port)->step;
	} else
		oPort->info.ptr = port->ptr;
}

opdi::Port* OPDI::findPort(opdi_Port* port) {
	if (port == nullptr)
		return *this->ports.begin();
	auto it = this->ports.begin();
	auto ite = this->ports.end();
	while (it != ite) {
		if ((opdi_Port*)(*it)->data == port)
			return *it;
		++it;
	}
	// not found
	return nullptr;
}

opdi::PortList& OPDI::getPorts() {
	return this->ports;
}

opdi::Port* OPDI::findPortByID(const char* portID, bool caseInsensitive) {
	auto it = this->ports.begin();
	auto ite = this->ports.end();
	while (it != ite) {
		opdi_Port* oPort = (opdi_Port*)(*it)->data;
		if (caseInsensitive) {
#ifdef linux
			if (strcasecmp(oPort->id, portID) == 0)
#else
			if (strcmpi(oPort->id, portID) == 0)
#endif
				return *it;
		} else {
			if (strcmp(oPort->id, portID) == 0)
				return *it;
		}
		++it;
	}
	// not found
	return nullptr;
}

void OPDI::updatePortGroupData(opdi::PortGroup *group) {
	// allocate port group data structure if necessary
	opdi_PortGroup *oGroup = (opdi_PortGroup *)group->data;
	if (oGroup == nullptr) {
		oGroup = (opdi_PortGroup *)malloc(sizeof(opdi_PortGroup));
		group->data = oGroup;
		oGroup->next = nullptr;
	}
	// update data
	oGroup->id = (const char*)group->id;
	oGroup->label = (const char*)group->label;
	oGroup->parent = (const char*)group->parent;
	oGroup->flags = group->flags;
	oGroup->extendedInfo = group->extendedInfo;
}

void OPDI::addPortGroup(opdi::PortGroup *portGroup) {
	// associate port with this instance
	portGroup->opdi = this;
	/*
	// first added port?
	if (this->first_portGroup == nullptr) {
		this->first_portGroup = portGroup;
		this->last_portGroup = portGroup;
	} else {
		// subsequently added port group, add to list
		this->last_portGroup->next = portGroup;
		this->last_portGroup = portGroup;
	}
	*/
	this->groups.push_back(portGroup);
	this->updatePortGroupData(portGroup);

	opdi_add_portgroup((opdi_PortGroup*)portGroup->data);
}

opdi::PortGroupList& OPDI::getPortGroups(void) {
	return this->groups;
}


void OPDI::sortPorts(void) {
	std::sort(this->ports.begin(), this->ports.end(), [] (opdi::Port* i, opdi::Port* j) { return i->orderID < j->orderID; });
}

void OPDI::preparePorts(void) {
	auto it = this->ports.begin();
	auto ite = this->ports.end();
	while (it != ite) {
		(*it)->prepare();

		// add ports to the OPDI C subsystem; ignore hidden ports
		if (!(*it)->isHidden()) {
			int result = opdi_add_port((opdi_Port*)(*it)->data);
			if (result != OPDI_STATUS_OK)
				throw Poco::ApplicationException("Unable to add port: " + (*it)->ID() + "; code = " + (*it)->to_string(result));
		}
		++it;
	}
}

uint8_t OPDI::start() {
	opdi_Message message;
	uint8_t result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK) {
		return result;
	}

	// reset idle timer
	this->last_activity = opdi_get_time_ms();

	// initiate handshake
	result = opdi_slave_start(&message, nullptr, nullptr);

	return result;
}

uint8_t OPDI::waiting(uint8_t canSend) {
	if (this->shutdownRequested) {
		return this->shutdownInternal();
	}

	// remember canSend flag
	this->canSend = canSend;

	// call ports' doWork function
	auto it = this->ports.begin();
	auto ite = this->ports.end();
	while (it != ite) {
		uint8_t result = (*it)->doWork(canSend);
		if (result != OPDI_STATUS_OK)
			return result;
		++it;
	}
	return OPDI_STATUS_OK;
}

uint8_t OPDI::isConnected() {
	return opdi_slave_connected();
}

uint8_t OPDI::disconnect() {
	if (!this->isConnected() || !this->canSend) {
		return OPDI_DISCONNECTED;
	}
	return opdi_disconnect();
}

uint8_t OPDI::reconfigure() {
	if (!this->isConnected() || !this->canSend)
		return OPDI_DISCONNECTED;
	return opdi_reconfigure();
}

uint8_t OPDI::refresh(opdi::Port** ports) {
	if (!this->isConnected() || !this->canSend)
		return OPDI_DISCONNECTED;
	opdi_Port* iPorts[OPDI_MAX_MESSAGE_PARTS + 1];
	iPorts[0] = nullptr;
	if (ports == nullptr)
		return opdi_refresh(iPorts);
	// target array of internal ports to refresh
	opdi::Port* port = ports[0];
	uint8_t i = 0;
	while (port != nullptr) {
		opdi_Port* oPort = (opdi_Port*)port->data;
		iPorts[i] = oPort;
		if (++i > OPDI_MAX_MESSAGE_PARTS)
			return OPDI_ERROR_PARTS_OVERFLOW;
		port = ports[i];
	}
	iPorts[i] = nullptr;
	return opdi_refresh(iPorts);
}

uint8_t OPDI::idleTimeoutReached() {
	if (this->isConnected() && this->canSend) {
		opdi_send_debug("Idle timeout!");
	}
	return this->disconnect();
}

uint8_t OPDI::messageHandled(channel_t channel, const char** /*parts*/) {
	// a complete message has been processed; it's now safe to send
	this->canSend = true;

	// idle timeout detection active?
	if (this->idle_timeout_ms > 0) {
		// Channels above a given threshold do reset the activity time.
		// Messages on channels below this do not reset it. Usually the
		// channel assignments are as follows:
		// 0: control channel
		// 1: refresh channel
		// 2 - 19: reserved for streaming ports
		// 20 and more: user interaction channels
		// We want to reset the activity only if a "real" user interaction occurs.
		// That means, ping messages and automatic refreshes caused by the device
		// should not cause the device to be connected indefinitely.
		// TODO find a better way to specify this (masters must respect this convention)
		if (channel >= 20) {
			// reset activity time
			this->last_activity = opdi_get_time_ms();
		} else {
			// non-resetting message

			// check idle timeout
			if (opdi_get_time_ms() - this->last_activity > this->idle_timeout_ms) {
				return this->idleTimeoutReached();
			}
		}
	}

	return OPDI_STATUS_OK;
}

void OPDI::shutdown(void) {
	// set flag to indicate that the message processing should stop
	// this method may be called asynchronously by signal handlers or threads,
	// so it is very important not to do any actual work here
	this->shutdownRequested = true;
}

void OPDI::persist(opdi::Port*  /*port*/) {
	throw Poco::NotImplementedException("This implementation does not support port state persistance");
}

opdi::Port* OPDI::findPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	return port;
}

void OPDI::findPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, opdi::PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		if (item == "*") {
			// add all ports
			portList = this->getPorts();
		}
		else
			// ignore empty items
			if (!item.empty()) {
				opdi::Port* port = this->findPort(configPort, setting, item, true);
				if (port != nullptr)
					portList.push_back(port);
			}
	}
}

opdi::DigitalPort* OPDI::findDigitalPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "digital"
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (opdi::DigitalPort*)port;
}

void OPDI::findDigitalPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, opdi::DigitalPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			opdi::DigitalPort* port = this->findDigitalPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

opdi::AnalogPort* OPDI::findAnalogPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "analog"
	if (port->getType()[0] != OPDI_PORTTYPE_ANALOG[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not an analog port: " + portID);

	return (opdi::AnalogPort*)port;
}

void OPDI::findAnalogPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, opdi::AnalogPortList& portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (!item.empty()) {
			opdi::AnalogPort* port = this->findAnalogPort(configPort, setting, item, true);
			if (port != nullptr)
				portList.push_back(port);
		}
	}
}

opdi::SelectPort* OPDI::findSelectPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required) {
	// locate port by ID
	opdi::Port* port = this->findPortByID(portID.c_str());
	// no found but required?
	if (port == nullptr) {
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return nullptr;
	}

	// port type must be "select"
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a select port: " + portID);

	return (opdi::SelectPort*)port;
}

void OPDI::logWarning(const std::string& message) {
	if (this->logVerbosity >= LogVerbosity::NORMAL) {
		this->logWarn(message);
	}
}

void OPDI::logError(const std::string& message) {
	this->logErr(message);
}

void OPDI::logNormal(const std::string& message, LogVerbosity verbosity) {
	// supplied log verbosity takes precedence
	LogVerbosity lv = (verbosity != LogVerbosity::UNKNOWN ? verbosity : this->logVerbosity);
	if (lv >= LogVerbosity::NORMAL) {
		this->log(message);
	}
}

void OPDI::logVerbose(const std::string& message, LogVerbosity verbosity) {
	// supplied log verbosity takes precedence
	LogVerbosity lv = (verbosity != LogVerbosity::UNKNOWN ? verbosity : this->logVerbosity);
	if (lv >= LogVerbosity::VERBOSE) {
		this->log(message);
	}
}

void OPDI::logDebug(const std::string& message, LogVerbosity verbosity) {
	// supplied log verbosity takes precedence
	LogVerbosity lv = (verbosity != LogVerbosity::UNKNOWN ? verbosity : this->logVerbosity);
	if (lv >= LogVerbosity::DEBUG) {
		this->log(message);
	}
}

void OPDI::logExtreme(const std::string& message, LogVerbosity verbosity) {
	// supplied log verbosity takes precedence
	LogVerbosity lv = (verbosity != LogVerbosity::UNKNOWN ? verbosity : this->logVerbosity);
	if (lv >= LogVerbosity::EXTREME) {
		this->log(message);
	}
}

double OPDI::getPortValue(opdi::Port* port) const {
	double value = 0;

	// evaluation depends on port type
	if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
		// digital port: Low = 0; High = 1
		uint8_t mode;
		uint8_t line;
		((opdi::DigitalPort*)port)->getState(&mode, &line);
		value = line;
	}
	else
		if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
			// analog port: relative value (0..1)
			value = ((opdi::AnalogPort*)port)->getRelativeValue();
		}
		else
			if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
				// dial port: absolute value
				int64_t position;
				((opdi::DialPort*)port)->getState(&position);
				value = (double)position;
			}
			else
				if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
					// select port: current position number
					uint16_t position;
					((opdi::SelectPort*)port)->getState(&position);
					value = position;
				}
				else
					// port type not supported
					throw Poco::Exception("Port type not supported");

	return value;
}



}		// namespace opdi
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
 * Uses serial port communication.
 */

#include <inttypes.h>
#include <string.h>

#include "opdi_constants.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "OPDI.h"

uint16_t opdi_device_flags = 0;

// digital port functions
#ifndef OPDI_NO_DIGITAL_PORTS

uint8_t opdi_get_digital_port_state(opdi_Port* port, char mode[], char line[]) {
	uint8_t result;
	uint8_t dMode;
	uint8_t dLine;
	OPDI_DigitalPort* dPort = (OPDI_DigitalPort*)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	result = dPort->getState(&dMode, &dLine);
	if (result != OPDI_STATUS_OK)
		return result;
	mode[0] = '0' + dMode;
	line[0] = '0' + dLine;

	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port* port, const char line[]) {
	uint8_t dLine;

	OPDI_DigitalPort* dPort = (OPDI_DigitalPort*)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if (line[0] == '1')
		dLine = 1;
	else
		dLine = 0;
	return dPort->setLine(dLine);
}

uint8_t opdi_set_digital_port_mode(opdi_Port* port, const char mode[]) {
	uint8_t dMode;

	OPDI_DigitalPort* dPort = (OPDI_DigitalPort*)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((mode[0] >= '0') && (mode[0] <= '3'))
		dMode = mode[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return dPort->setMode(dMode);
}

#endif 		// OPDI_NO_DIGITAL_PORTS

#ifndef OPDI_NO_ANALOG_PORTS

uint8_t opdi_get_analog_port_state(opdi_Port* port, char mode[], char res[], char ref[], int32_t* value) {
	uint8_t result;
	uint8_t aMode;
	uint8_t aRef;
	uint8_t aRes;

	OPDI_AnalogPort* aPort = (OPDI_AnalogPort*)Opdi->findPort(port);
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

uint8_t opdi_set_analog_port_value(opdi_Port* port, int32_t value) {
	OPDI_AnalogPort* aPort = (OPDI_AnalogPort*)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return aPort->setValue(value);
}

uint8_t opdi_set_analog_port_mode(opdi_Port* port, const char mode[]) {
	uint8_t aMode;

	OPDI_AnalogPort* aPort = (OPDI_AnalogPort*)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((mode[0] >= '0') && (mode[0] <= '1'))
		aMode = mode[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setMode(aMode);
}

uint8_t opdi_set_analog_port_resolution(opdi_Port* port, const char res[]) {
	uint8_t aRes;
	OPDI_AnalogPort* aPort = (OPDI_AnalogPort*)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((res[0] >= '0') && (res[0] <= '4'))
		aRes = res[0] - '0' + 8;
	else
		// resolution not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setResolution(aRes);
}

uint8_t opdi_set_analog_port_reference(opdi_Port* port, const char ref[]) {
	uint8_t aRef;
	OPDI_AnalogPort* aPort = (OPDI_AnalogPort*)Opdi->findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	if ((ref[0] >= '0') && (ref[0] <= '1'))
		aRef = ref[0] - '0';
	else
		// mode not supported
		return OPDI_PROTOCOL_ERROR;

	return aPort->setReference(aRef);
}

#endif		// OPDI_NO_ANALOG_PORTS

#ifndef OPDI_NO_SELECT_PORTS

uint8_t opdi_get_select_port_state(opdi_Port* port, uint16_t* position) {
	OPDI_SelectPort* sPort = (OPDI_SelectPort*)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return sPort->getState(position);
}

uint8_t opdi_set_select_port_position(opdi_Port* port, uint16_t position) {
	OPDI_SelectPort* sPort = (OPDI_SelectPort*)Opdi->findPort(port);
	if (sPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return sPort->setPosition(position);
}

#endif	//  OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

uint8_t opdi_get_dial_port_state(opdi_Port* port, int64_t* position) {
	OPDI_DialPort* dPort = (OPDI_DialPort*)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return dPort->getState(position);
}

uint8_t opdi_set_dial_port_position(opdi_Port* port, int64_t position) {
	OPDI_DialPort* dPort = (OPDI_DialPort*)Opdi->findPort(port);
	if (dPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return dPort->setPosition(position);
}

#endif	// OPDI_NO_DIAL_PORTS


// general functions

uint8_t opdi_choose_language(const char* languages) {
	// supports German?
	if (strcmp("de_DE", languages) == 0) {
		// TODO
	}

	return OPDI_STATUS_OK;
}

#ifdef OPDI_HAS_MESSAGE_HANDLED

uint8_t opdi_message_handled(channel_t channel, const char** parts) {
	return Opdi->messageHandled(channel, parts);
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////
// General port functionality
//////////////////////////////////////////////////////////////////////////////////////////

OPDI_Port::OPDI_Port(const char* id, const char* name, const char* type, const char* dircaps) {
	// protected constructor
	// assign buffers, clear fields
	this->port.id = (const char*)&this->id;
	this->port.name = (const char*)&this->name;
	this->port.type = (const char*)&this->type;
	this->port.caps = (const char*)&this->caps;
	this->port.info.i = 0;
	this->port.flags = 0;
	this->port.next = NULL;
	this->next = NULL;

	// copy ID to class buffer
	strncpy(this->id, id, (MAX_PORTIDLENGTH) - 1);
	this->setName(name);
	strcpy(this->type, type);
	strcpy(this->caps, dircaps);
}

uint8_t OPDI_Port::doWork() {
	return OPDI_STATUS_OK;
}

void OPDI_Port::setName(const char* name) {
	// copy name to class buffer
	strncpy(this->name, name, (MAX_PORTNAMELENGTH) - 1);
}

uint8_t OPDI_Port::refresh() {
	OPDI_Port* ports[2];
	ports[0] = this;
	ports[1] = NULL;

	return Opdi->refresh(ports);
}

uint8_t OPDI_Port::getExtendedInfo(char* buffer, size_t length) {
	// standard implementation returns an empty string
	buffer[0] = '\0';
	return OPDI_STATUS_OK;
}

OPDI_Port::~OPDI_Port() {
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_DigitalPort::OPDI_DigitalPort(const char* id, const char* name, const char* dircaps, const int32_t flags) :
	// call base constructor
	OPDI_Port(id, name, OPDI_PORTTYPE_DIGITAL, dircaps) {

	this->port.flags = flags;
}

OPDI_DigitalPort::~OPDI_DigitalPort() {
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AnalogPort::OPDI_AnalogPort(const char* id, const char* name, const char* dircaps, const int32_t flags) :
	// call base constructor
	OPDI_Port(id, name, OPDI_PORTTYPE_ANALOG, dircaps) {

	this->port.flags = flags;
}

OPDI_AnalogPort::~OPDI_AnalogPort() {
}

#endif		// NO_ANALOG_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Dial port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIAL_PORTS

OPDI_DialPort::OPDI_DialPort(const char* id, const char* label, const int64_t minValue, const int64_t maxValue, const uint64_t step, const int32_t flags)
	: OPDI_Port(id, label, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT) {
	this->portInfo.min = minValue;
	this->portInfo.max = maxValue;
	this->portInfo.step = step;
	this->port.flags = flags;
	this->port.info.ptr = &this->portInfo;
}

OPDI_DialPort::~OPDI_DialPort() {}

#endif // OPDI_NO_DIAL_PORTS



//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t OPDI::setup(int16_t deviceFlags, Stream* ioStream, Stream* debugStream) {
	this->ioStream = ioStream;
	this->debugStream = debugStream;

	// initialize linked list of ports
	this->first_port = NULL;
	this->last_port = NULL;

	this->workFunction = NULL;
	opdi_device_flags = deviceFlags;

	opdi_slave_init();

	return OPDI_STATUS_OK;
}

void OPDI::setIdleTimeout(uint32_t idleTimeoutMs) {
	this->idle_timeout_ms = idleTimeoutMs;
}

uint8_t OPDI::addPort(OPDI_Port* port) {
	// first added port?
	if (this->first_port == NULL) {
		this->first_port = port;
		this->last_port = port;
	} else {
		// subsequently added port, add to list
		this->last_port->next = port;
		this->last_port = port;
	}

	return opdi_add_port(&port->port);
}

OPDI_Port* OPDI::findPort(opdi_Port* port) {

	OPDI_Port* p = this->first_port;
	// go through linked list
	while (p != NULL) {
		if (&p->port == port)
			return p;
		p = p->next;
	}
	// not found
	return NULL;
}

// convenience method
uint8_t OPDI::start() {
	return this->start(NULL);
}

uint8_t OPDI::start(uint8_t (*workFunction)()) {
	opdi_Message message;
	uint8_t result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK) {
		return result;
	}

	// reset idle timer
	this->last_activity = this->getTimeMs();
	this->workFunction = workFunction;

	// initiate handshake
	result = opdi_slave_start(&message, NULL, NULL);

	return result;
}

uint8_t OPDI::waiting(uint8_t canSend) {
	// a work function can be performed as long as canSend is true
	if ((this->workFunction != NULL) && canSend) {
		uint8_t result = this->workFunction();
		if (result != OPDI_STATUS_OK)
			return result;
	}

	// ports' doWork function can be called as long as canSend is true
	if (canSend) {
		OPDI_Port* p = this->first_port;
		// go through ports
		while (p != NULL) {
			// call doWork function, return errors immediately
			uint8_t result = p->doWork();
			if (result != OPDI_STATUS_OK)
				return result;
			p = p->next;
		}
	}

	return OPDI_STATUS_OK;
}

uint8_t OPDI::isConnected() {
	return opdi_slave_connected();
}

uint8_t OPDI::disconnect() {
	return opdi_disconnect();
}

uint8_t OPDI::reconfigure() {
	return opdi_reconfigure();
}

uint8_t OPDI::refresh(OPDI_Port** ports) {
	// target array of internal ports to refresh
	opdi_Port* iPorts[OPDI_MAX_MESSAGE_PARTS + 1];
	OPDI_Port* port = ports[0];
	uint8_t i = 0;
	while (port != NULL) {
		iPorts[i++] = &port->port;
		port = ports[i];
		if (i > OPDI_MAX_MESSAGE_PARTS)
			return OPDI_ERROR_PARTS_OVERFLOW;
	}
	iPorts[i] = NULL;
	return opdi_refresh(iPorts);
}

uint8_t OPDI::messageHandled(channel_t channel, const char** parts) {
	if (this->idle_timeout_ms > 0) {
		// channels below 20 do not reset the activity timer
		// (control messages and device-caused refreshes)
		if (channel >= 20) {
			// reset activity time
			this->last_activity = this->getTimeMs();
		} else {
			// control channel message
			if (this->getTimeMs() - this->last_activity > this->idle_timeout_ms) {
				opdi_send_debug("Idle timeout!");
				return this->disconnect();
			}
		}
	}

	return OPDI_STATUS_OK;
}


void OPDI::getSlaveName(char* buffer, size_t length) {
	strncpy(buffer, "OPDI Slave", length);
}

void OPDI::getEncoding(char* buffer, size_t length) {
	strncpy(buffer, "utf-8", length);
}

uint8_t OPDI::setLanguages(char* languages) {
	return OPDI_STATUS_OK;
}

uint8_t OPDI::setUsername(char* username) {
	return OPDI_STATUS_OK;
}

uint8_t OPDI::setPassword(char* password) {
	return OPDI_STATUS_OK;
}

uint8_t OPDI::getExtendedPortInfo(char* buffer, size_t length) {
	OPDI_Port* port = this->first_port;
	// go through linked list
	while (port != NULL) {
		if (!strcmp(port->id, buffer)) break;
		port = port->next;
	}
	if (port == NULL) {
		return OPDI_PORT_UNKNOWN;
	} else {
		return port->getExtendedInfo(buffer, length);
	}
}


uint8_t opdi_slave_callback(OPDIFunctionCode opdiFunctionCode, char* buffer, size_t data) {

	switch (opdiFunctionCode) {
	case OPDI_FUNCTION_GET_CONFIG_NAME: Opdi->getSlaveName(buffer, data); return OPDI_STATUS_OK;
	case OPDI_FUNCTION_SET_MASTER_NAME: return OPDI_STATUS_OK;
	case OPDI_FUNCTION_GET_SUPPORTED_PROTOCOLS: strncpy(buffer, "EP,BP", data); return OPDI_STATUS_OK;
	case OPDI_FUNCTION_GET_ENCODING: Opdi->getEncoding(buffer, data); return OPDI_STATUS_OK;
	case OPDI_FUNCTION_SET_LANGUAGES: return Opdi->setLanguages(buffer);
	case OPDI_FUNCTION_GET_EXTENDED_DEVICEINFO: buffer[0] = '\0'; return OPDI_STATUS_OK;
	case OPDI_FUNCTION_GET_EXTENDED_PORTINFO: {
		uint8_t code = Opdi->getExtendedPortInfo(buffer, data);
		return OPDI_STATUS_OK;
	}
	case OPDI_FUNCTION_GET_EXTENDED_PORTSTATE: buffer[0] = '\0'; return OPDI_STATUS_OK;
#ifndef OPDI_NO_AUTHENTICATION
	case OPDI_FUNCTION_SET_USERNAME: return Opdi->setUsername(buffer);
	case OPDI_FUNCTION_SET_PASSWORD: return Opdi->setPassword(buffer);
#endif
	default: return OPDI_FUNCTION_UNKNOWN;
	}
}

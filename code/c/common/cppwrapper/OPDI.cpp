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

#include <string.h>

#include <opdi_constants.h>
#include <opdi_protocol.h>
#include <opdi_slave_protocol.h>
#include <opdi_config.h>

#include <opdi_configspecs.h>
#include <opdi_platformfuncs.h>

#include "OPDI.h"

//////////////////////////////////////////////////////////////////////////////////////////
// General port functionality
//////////////////////////////////////////////////////////////////////////////////////////

OPDI_Port::OPDI_Port(const char *id, const char *name, const char *type, const char *dircaps) {
	// protected constructor
	// assign buffers, clear fields
	this->port.id = (const char*)&this->id;
	this->port.name = (const char*)&this->name;
	this->port.type = (const char*)&this->type;
	this->port.caps = (const char*)&this->caps;
	this->port.info.i = 0;
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

void OPDI_Port::setName(const char *name) {
	// copy name to class buffer
	strncpy(this->name, name, (MAX_PORTNAMELENGTH) - 1);
}

uint8_t OPDI_Port::refresh() {
	OPDI_Port **ports = new OPDI_Port*[2];
	ports[0] = this;
	ports[1] = NULL;

	return opdi->refresh(ports);
}

OPDI_Port::~OPDI_Port() {
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_DigitalPort::OPDI_DigitalPort(const char *id, const char *name, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, name, OPDI_PORTTYPE_DIGITAL, dircaps) {

	this->port.info.i = flags;
}

OPDI_DigitalPort::~OPDI_DigitalPort() {
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AnalogPort::OPDI_AnalogPort(const char *id, const char *name, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, name, OPDI_PORTTYPE_ANALOG, dircaps) {

	this->port.info.i = flags;
}

OPDI_AnalogPort::~OPDI_AnalogPort() {
}

#endif		// NO_ANALOG_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t OPDI::setup(const char *slaveName, int idleTimeout) {
	// initialize linked list of ports
	this->first_port = NULL;
	this->last_port = NULL;

	this->workFunction = NULL;

	// copy slave name to internal buffer
	strncpy((char*)opdi_config_name, slaveName, MAX_SLAVENAMELENGTH - 1);
	// set standard encoding to "utf-8"
	strncpy((char*)opdi_encoding, "utf-8", MAX_ENCODINGNAMELENGTH - 1);

	this->idle_timeout_ms = idleTimeout;

	return opdi_slave_init();
}

void OPDI::setIdleTimeout(uint32_t idleTimeoutMs) {
	this->idle_timeout_ms = idleTimeoutMs;
}

uint8_t OPDI::addPort(OPDI_Port *port) {
	// associate port with this instance
	port->opdi = this;

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

OPDI_Port *OPDI::findPort(opdi_Port *port) {
	OPDI_Port *p = this->first_port;
	// go through linked list
	while (p != NULL) {
		if (&p->port == port)
			return p;
		p = p->next;
	}
	// not found
	return NULL;
}

OPDI_Port *OPDI::findPortByID(const char *portID) {
	OPDI_Port *p = this->first_port;
	// go through linked list
	while (p != NULL) {
		if (strcmp(p->port.id, portID) == 0)
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
	this->last_activity = opdi_get_time_ms();
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
		OPDI_Port *p = this->first_port;
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

uint8_t isConnected() {
	return opdi_slave_connected();
}

uint8_t OPDI::disconnect() {
	return opdi_disconnect();
}

uint8_t OPDI::reconfigure() {
	return opdi_reconfigure();
}

uint8_t OPDI::refresh(OPDI_Port **ports) {
	// target array of internal ports to refresh
	opdi_Port **iPorts = new opdi_Port*[OPDI_MAX_MESSAGE_PARTS + 1];
	OPDI_Port *port = ports[0];
	uint8_t i = 0;
	while (port != NULL) {
		iPorts[i] = &port->port;
		port = port->next;
		if (++i > OPDI_MAX_MESSAGE_PARTS)
			return OPDI_ERROR_PARTS_OVERFLOW;
	}
	iPorts[i] = NULL;
	return opdi_refresh(iPorts);
}

uint8_t OPDI::messageHandled(channel_t channel, const char **parts) {
	if (this->idle_timeout_ms > 0) {
		if (channel != 0) {
			// reset activity time
			this->last_activity = opdi_get_time_ms();
		} else {
			// control channel message
			if (opdi_get_time_ms() - this->last_activity > this->idle_timeout_ms) {
				opdi_send_debug("Idle timeout!");
				return this->disconnect();
			}
		}
	}

	return OPDI_STATUS_OK;
}

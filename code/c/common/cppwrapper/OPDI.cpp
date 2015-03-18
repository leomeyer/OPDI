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

#include <cstdlib>
#include <string.h>

#include "opdi_constants.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "opdi_configspecs.h"
#include "opdi_platformfuncs.h"

#include "OPDI.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t OPDI::shutdownInternal(void) {

	// free all ports
	OPDI_Port *port = this->first_port;
	this->first_port = NULL;
	this->last_port = NULL;
	while (port) {
		OPDI_Port *next = port->next;
		delete port;
		port = next;
	}

	return this->disconnect();
}

uint8_t OPDI::setup(const char *slaveName, int idleTimeout) {
	this->shutdownRequested = false;

	// initialize linked list of ports
	this->first_port = NULL;
	this->last_port = NULL;

	this->workFunction = NULL;

	// copy slave name to internal buffer
	strncpy((char*)opdi_config_name, slaveName, OPDI_MAX_SLAVENAMELENGTH - 1);
	// set standard encoding to "utf-8"
	strncpy((char*)opdi_encoding, "utf-8", OPDI_MAX_ENCODINGNAMELENGTH - 1);

	this->idle_timeout_ms = idleTimeout;

	return opdi_slave_init();
}

void OPDI::setIdleTimeout(uint32_t idleTimeoutMs) {
	this->idle_timeout_ms = idleTimeoutMs;
}

void OPDI::setEncoding(const char* encoding) {
	strncpy((char*)opdi_encoding, encoding, OPDI_MAX_ENCODINGNAMELENGTH - 1);
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

	this->updatePortData(port);

	return opdi_add_port((opdi_Port*)port->data);
}

// possible race conditions here, if one thread updates port data while the other retrieves it
// how to avoid this problem?
void OPDI::updatePortData(OPDI_Port *port) {
	// allocate port data structure if necessary
	opdi_Port *oPort = (opdi_Port *)port->data;
	if (oPort == NULL) {
		oPort = (opdi_Port *)malloc(sizeof(opdi_Port));
		port->data = oPort;
		oPort->info.i = 0;
		oPort->info.ptr = NULL;
		oPort->next = NULL;
	}
	// update data
	oPort->id = (const char*)port->id;
	oPort->name = (const char*)port->label;
	oPort->type = (const char*)port->type;
	oPort->caps = (const char*)port->caps;
	// for "simple" ports, update flags
	if (port->flags != 0)
		oPort->info.i = port->flags;
	else {
		// more complex ports require the pointer to contain additional information

		// check port type
		if (strcmp(port->type, OPDI_PORTTYPE_SELECT) == 0) {
			oPort->info.ptr = ((OPDI_SelectPort*)port)->items;
		} else
		if (strcmp(port->type, OPDI_PORTTYPE_DIAL) == 0) {
			// release additional data structure memory
			if (oPort->info.ptr != NULL)
				free(oPort->info.ptr);
			// allocate additional data structure memory
			struct opdi_DialPortInfo* dialPortInfo = (struct opdi_DialPortInfo*)malloc(sizeof(opdi_DialPortInfo));
			oPort->info.ptr = dialPortInfo;
			dialPortInfo->min = ((OPDI_DialPort*)port)->minValue;
			dialPortInfo->max = ((OPDI_DialPort*)port)->maxValue;
			dialPortInfo->step = ((OPDI_DialPort*)port)->step;
		} else
			oPort->info.ptr = port->ptr;
	}
}

OPDI_Port *OPDI::findPort(opdi_Port *port) {
	OPDI_Port *p = this->first_port;
	// go through linked list
	while (p != NULL) {
		if ((opdi_Port*)p->data == port)
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
		opdi_Port *oPort = (opdi_Port *)p->data;
		if (strcmp(oPort->id, portID) == 0)
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
	if (this->shutdownRequested) {
		return this->shutdownInternal();
	}

	// a work function can be performed as long as canSend is true
	if ((this->workFunction != NULL) && canSend) {
		uint8_t result = this->workFunction();
		if (result != OPDI_STATUS_OK)
			return result;
	}

	// call ports' doWork function
	OPDI_Port *p = this->first_port;
	// go through ports
	while (p != NULL) {
		// call doWork function, return errors immediately
		uint8_t result = p->doWork(canSend);
		if (result != OPDI_STATUS_OK)
			return result;
		p = p->next;
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
	if (!this->isConnected())
		return OPDI_DISCONNECTED;
	return opdi_reconfigure();
}

uint8_t OPDI::refresh(OPDI_Port **ports) {
	if (!this->isConnected())
		return OPDI_DISCONNECTED;
	// target array of internal ports to refresh
	opdi_Port **iPorts = new opdi_Port*[OPDI_MAX_MESSAGE_PARTS + 1];
	OPDI_Port *port = ports[0];
	uint8_t i = 0;
	while (port != NULL) {
		opdi_Port *oPort = (opdi_Port *)port->data;
		iPorts[i] = oPort;
		if (++i > OPDI_MAX_MESSAGE_PARTS)
			return OPDI_ERROR_PARTS_OVERFLOW;
		port = ports[i];
	}
	iPorts[i] = NULL;
	return opdi_refresh(iPorts);
}

uint8_t OPDI::idleTimeoutReached() {
	opdi_send_debug("Idle timeout!");
	return this->disconnect();
}

uint8_t OPDI::messageHandled(channel_t channel, const char **parts) {
	if (this->idle_timeout_ms > 0) {
		if (channel != 0) {
			// reset activity time
			this->last_activity = opdi_get_time_ms();
		} else {
			// control channel message

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
	this->shutdownRequested = true;
}
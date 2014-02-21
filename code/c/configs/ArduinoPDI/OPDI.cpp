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

#include <opdi_constants.h>
#include <opdi_protocol.h>
#include <opdi_slave_protocol.h>
#include <opdi_config.h>

#include <opdi_configspecs.h>

#include <WProgram.h>
#include "arduino.h";

#include "OPDI.h"

const char *opdi_encoding = new char[MAX_ENCODINGNAMELENGTH];
uint16_t opdi_device_flags = 0;
const char *opdi_supported_protocols = "BP";
const char *opdi_config_name = new char[MAX_SLAVENAMELENGTH];
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];

/** Receives a character from the UART and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {

	unsigned long ticks = millis();

	while (Serial.available() == 0) {
		// timed out?
		if ((millis() - ticks) > timeout)
			return OPDI_TIMEOUT;

		uint8_t result = Opdi.waiting(canSend);
		if (result != OPDI_STATUS_OK)
			return result;
	}
	uint16_t data = Serial.read();

	*byte = (uint8_t)data;

	return OPDI_STATUS_OK;
}

/** Sends count bytes to the serial port. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	Serial.write(bytes, count);

	return OPDI_STATUS_OK;
}

/** Can be used to send debug messages to a monitor.
 *
 */
uint8_t opdi_debug_msg(const uint8_t *message, uint8_t direction) {
	return OPDI_STATUS_OK;
}

// digital port functions
#ifndef NO_DIGITAL_PORTS

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	uint8_t result;
	uint8_t dMode;
	uint8_t dLine;
	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi.findPort(port);
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

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi.findPort(port);
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

	OPDI_DigitalPort *dPort = (OPDI_DigitalPort *)Opdi.findPort(port);
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

	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi.findPort(port);
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
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi.findPort(port);
	if (aPort == NULL)
		return OPDI_PORT_UNKNOWN;

	return aPort->setValue(value);
}

uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	uint8_t aMode;

	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi.findPort(port);
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
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi.findPort(port);
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
	OPDI_AnalogPort *aPort = (OPDI_AnalogPort *)Opdi.findPort(port);
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
	return Opdi.messageHandled(channel, parts);
}

#endif

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

void OPDI_Port::setName(const char *name) {
	// copy name to class buffer
	strncpy(this->name, name, (MAX_PORTNAMELENGTH) - 1);
}

uint8_t OPDI_Port::refresh() {
	OPDI_Port **ports = new OPDI_Port*[2];
	ports[0] = this;
	ports[1] = NULL;

	return Opdi.refresh(ports);
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

OPDI_DigitalPortPin::OPDI_DigitalPortPin(const char *id, const char *name, const char *dircaps, const uint8_t flags, const uint8_t pin) :
	// call base constructor; digital pins do not support pulldowns, mask the respective flags
	OPDI_DigitalPort(id, name, dircaps, flags & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS)) {

	this->pin = pin;
	this->line = 0;
	// set mode depending on dircaps and flags
	if (!strcmp(dircaps, OPDI_PORTDIRCAP_INPUT) || !strcmp(dircaps, OPDI_PORTDIRCAP_BIDI))  {
		if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) == OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
			mode = 1;
		else
			mode = 0;
	} else
		// mode is output
		mode = 3;

	// configure pin
	this->setMode(mode);
}

OPDI_DigitalPortPin::~OPDI_DigitalPortPin() {
}

// function that handles the set direction command (opdi_set_digital_port_mode)
uint8_t OPDI_DigitalPortPin::setMode(uint8_t mode) {
	if (mode == 0) {
		// set pin direction to input, floating
		pinMode(this->pin, INPUT);
		digitalWrite(pin, LOW);		// disable pullup
	} else
	if (mode == 1) {
		// set pin direction to input, pullup on
		pinMode(this->pin, INPUT);
		digitalWrite(pin, HIGH);		// enable pullup
	}
	else
	// mode == 2 is not possible because Arduino has no internal pulldowns
	if (mode == 3) {
		// set pin direction to output
		pinMode(this->pin, OUTPUT);
		digitalWrite(pin, LOW);
		// when setting pin as output, the line is cleared
		this->line = 0;
	}
	else
		// the device does not support the setting
		return OPDI_DEVICE_ERROR;

	this->mode = mode;
	return OPDI_STATUS_OK;
}

// function that handles the set line command (opdi_set_digital_port_line)
uint8_t OPDI_DigitalPortPin::setLine(uint8_t line) {
	if (this->mode == 3) {
		digitalWrite(pin, (line == 1 ? HIGH : LOW));
	} else
		// cannot set line if not in output mode
		return OPDI_DEVICE_ERROR;

	this->line = line;

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
uint8_t OPDI_DigitalPortPin::getState(uint8_t *mode, uint8_t *line) {
	// output?
	if (this->mode == 3) {
		// set remembered line
		*line = this->line;
	} else {
		// read pin
		*line = digitalRead(pin);
	}
	*mode = this->mode;

	return OPDI_STATUS_OK;
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

OPDI_AnalogPortPin::OPDI_AnalogPortPin(const char *id, const char *name, const char *dircaps, const uint8_t flags, const uint8_t pin) :
	// call base constructor; analog ports can't change resolutions, mask the respective flags
	OPDI_AnalogPort(id, name, dircaps, flags & ~(OPDI_ANALOG_PORT_CAN_CHANGE_RES)) {

	this->pin = pin;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
	// set reference depending on dircaps and flags
	if (!strcmp(dircaps, OPDI_PORTDIRCAP_INPUT) || !strcmp(dircaps, OPDI_PORTDIRCAP_BIDI))  {
		// default mode is input if possible
		mode = 0;
	} else {
		// mode is output
		mode = 1;
	}
	// configure pin; sets the resolution variable too
	this->setMode(mode);
}

OPDI_AnalogPortPin::~OPDI_AnalogPortPin() {
}

// function that handles the set direction command (opdi_set_digital_port_mode)
uint8_t OPDI_AnalogPortPin::setMode(uint8_t mode) {
	// input?
	if (mode == 0) {
		resolution = 10;	// 10 bits for inputs
		// perform a read - this should disable PWM of possible previous output pin
		value = analogRead(pin);
	} else
	if (mode == 1) {
		resolution = 8;	// 8 bits for outputs
		// no need to configure analog outputs
		value = 0;
	}
	else
		// the device does not support the setting
		return OPDI_DEVICE_ERROR;

	this->mode = mode;
	return OPDI_STATUS_OK;
}

uint8_t OPDI_AnalogPortPin::setResolution(uint8_t resolution) {
	// setting the resolution is not allowed
	return OPDI_DEVICE_ERROR;
}

uint8_t OPDI_AnalogPortPin::setReference(uint8_t reference) {
	this->reference = reference;

	if (reference == 0) {
		analogReference(DEFAULT);
	}
	else {
		analogReference(EXTERNAL);
	}

	return OPDI_STATUS_OK;
}

uint8_t OPDI_AnalogPortPin::setValue(int32_t value) {
	// not for inputs
	if (this->mode == 0)
		return OPDI_DEVICE_ERROR;

	// restrict input to possible values
	this->value = value & ((1 << this->resolution) - 1);

	analogWrite(pin, this->value);

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
uint8_t OPDI_AnalogPortPin::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) {
	*mode = this->mode;
	*resolution = this->resolution;
	*reference = this->reference;

	// output?
	if (this->mode == 1) {
		// set remembered value
		*value = this->value;
	} else {
		// read pin
		*value = analogRead(pin);
	}

	return OPDI_STATUS_OK;
}

#endif		// NO_ANALOG_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t OPDI::setup(const char *slaveName) {
	// initialize linked list of ports
	this->first_port = NULL;
	this->last_port = NULL;

	this->workFunction = NULL;

	// copy slave name to internal buffer
	strncpy((char*)opdi_config_name, slaveName, MAX_SLAVENAMELENGTH - 1);
	// set standard encoding to "utf-8"
	strncpy((char*)opdi_encoding, "utf-8", MAX_ENCODINGNAMELENGTH - 1);

	opdi_slave_init();

	return opdi_message_setup(&io_receive, &io_send, NULL);
}

void OPDI::setIdleTimeout(uint32_t idleTimeoutMs) {
	this->idle_timeout_ms = idleTimeoutMs;
}

uint8_t OPDI::addPort(OPDI_Port *port) {
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
	this->last_activity = millis();
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
			this->last_activity = millis();
		} else {
			// control channel message
			if (millis() - this->last_activity > this->idle_timeout_ms) {
				opdi_send_debug("Idle timeout!");
				return this->disconnect();
			}
		}
	}

	return OPDI_STATUS_OK;
}

// define global class instance
OPDI Opdi;

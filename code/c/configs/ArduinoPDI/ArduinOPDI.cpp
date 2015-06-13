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

/** OPDI C++ wrapper implementation for Arduino.
 * Uses serial port communication.
 */

#include <inttypes.h>
#include <string.h>

#include <opdi_constants.h>
#include <opdi_protocol.h>
#include <opdi_slave_protocol.h>
#include <opdi_config.h>

#include <opdi_configspecs.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "memory.h"

#include "ArduinOPDI.h"

/** Receives a character from the UART and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {

	unsigned long ticks = millis();

	while (Opdi->ioStream->available() == 0) {
		// timed out?
		if ((millis() - ticks) > timeout)
			return OPDI_TIMEOUT;

		uint8_t result = Opdi->waiting(canSend);
		if (result != OPDI_STATUS_OK)
			return result;
	}
	uint16_t data = Opdi->ioStream->read();

	*byte = (uint8_t)data;

	return OPDI_STATUS_OK;
}

/** Sends count bytes to the serial port. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	Opdi->ioStream->write(bytes, count);

	return OPDI_STATUS_OK;
}

/** Can be used to send debug messages to a monitor.
 *
 */
uint8_t opdi_debug_msg(const char *message, uint8_t direction) {
	if (Opdi->debugStream != NULL) {

		if (direction == OPDI_DIR_INCOMING)
			Opdi->debugStream->print(">");
		else
		if (direction == OPDI_DIR_OUTGOING)
			Opdi->debugStream->print("<");
#ifndef OPDI_NO_ENCRYPTION
		else
		if (direction == OPDI_DIR_INCOMING_ENCR)
			Opdi->debugStream->print("}");
		else
		if (direction == OPDI_DIR_OUTGOING_ENCR)
			Opdi->debugStream->print("{");
#endif
		else
			Opdi->debugStream->print("-");
		Opdi->debugStream->println(message);

	}
	return OPDI_STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Device specific OPDI implementation
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t ArduinOPDI::setup(const char *slaveName, uint32_t idleTimeout, int16_t deviceFlags, Stream* ioStream, Stream* debugStream) {
	uint8_t result = OPDI::setup(slaveName, deviceFlags, ioStream, debugStream);
	if (result != OPDI_STATUS_OK)
		return result;

	this->idle_timeout_ms = idleTimeout;

	// setup OPDI messaging via serial port
	return opdi_message_setup(&io_receive, &io_send, NULL);
}

ArduinOPDI::~ArduinOPDI() {
}

uint32_t ArduinOPDI::getTimeMs() {
	return millis();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

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

#ifndef OPDI_NO_ANALOG_PORTS

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

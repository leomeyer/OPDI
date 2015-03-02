

#include <string.h>

#include "opdi_constants.h"

#include "EmulatedPorts.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_EmulatedDigitalPort::OPDI_EmulatedDigitalPort(const char *id, const char *name, const char *dircaps, const uint8_t flags) :
	// call base constructor; mask unsupported flags
	OPDI_DigitalPort(id, name, dircaps, flags & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) & (OPDI_DIGITAL_PORT_HAS_PULLDN | OPDI_DIGITAL_PORT_PULLDN_ALWAYS)) {

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

OPDI_EmulatedDigitalPort::~OPDI_EmulatedDigitalPort() {
}

// function that handles the set direction command (opdi_set_digital_port_mode)
uint8_t OPDI_EmulatedDigitalPort::setMode(uint8_t mode) {
	if (mode == 0) {
		// set pin direction to input, floating
	} else
	if (mode == 1) {
		// set pin direction to input, pullup on
	}
	else
	if (mode == 2) {
		// set pin direction to input, pulldown on
	}
	if (mode == 3) {
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
uint8_t OPDI_EmulatedDigitalPort::setLine(uint8_t line) {
	if (this->mode == 3) {
	} else
		// cannot set line if not in output mode
		return OPDI_DEVICE_ERROR;

	this->line = line;

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
uint8_t OPDI_EmulatedDigitalPort::getState(uint8_t *mode, uint8_t *line) {
	// output?
	if (this->mode == 3) {
		// set remembered line
		*line = this->line;
	} else {
		// read pin
		*line = this->line;
	}
	*mode = this->mode;

	return OPDI_STATUS_OK;
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_EmulatedAnalogPort::OPDI_EmulatedAnalogPort(const char *id, const char *name, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_AnalogPort(id, name, dircaps, flags) {

	this->mode = 0;
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

OPDI_EmulatedAnalogPort::~OPDI_EmulatedAnalogPort() {
}

// function that handles the set direction command (opdi_set_digital_port_mode)
uint8_t OPDI_EmulatedAnalogPort::setMode(uint8_t mode) {
	// input?
	if (mode == 0) {
	} else
	// output?
	if (mode == 1) {
		value = 0;
	}
	else
		// the device does not support the setting
		return OPDI_DEVICE_ERROR;

	this->mode = mode;
	return OPDI_STATUS_OK;
}

uint8_t OPDI_EmulatedAnalogPort::setResolution(uint8_t resolution) {
	this->resolution = resolution;
	return OPDI_STATUS_OK;
}

uint8_t OPDI_EmulatedAnalogPort::setReference(uint8_t reference) {
	this->reference = reference;
	return OPDI_STATUS_OK;
}

uint8_t OPDI_EmulatedAnalogPort::setValue(int32_t value) {
	// not for inputs
	if (this->mode == 0)
		return OPDI_DEVICE_ERROR;

	// restrict input to possible values
	this->value = value & ((1 << this->resolution) - 1);

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
uint8_t OPDI_EmulatedAnalogPort::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) {
	*mode = this->mode;
	*resolution = this->resolution;
	*reference = this->reference;

	// output?
	if (this->mode == 1) {
		// set remembered value
		*value = this->value;
	} else {
		*value = this->value;
	}

	return OPDI_STATUS_OK;
}

#endif		// NO_ANALOG_PORTS

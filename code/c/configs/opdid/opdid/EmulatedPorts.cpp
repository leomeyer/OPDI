

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

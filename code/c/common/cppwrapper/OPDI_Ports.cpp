
#include <cstdlib>
#include <string.h>

#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_platformtypes.h"

#include "OPDI.h"
#include "OPDI_Ports.h"

//////////////////////////////////////////////////////////////////////////////////////////
// General port functionality
//////////////////////////////////////////////////////////////////////////////////////////

OPDI_Port::OPDI_Port(const char *id, const char *label, const char *type, const char *dircaps, int32_t flags, void* ptr) {
	this->data = NULL;
	this->next = NULL;
	this->id = NULL;
	this->label = NULL;
	this->opdi = NULL;
	this->flags = flags;
	this->ptr = ptr;

	this->id = (char*)malloc(strlen(id) + 1);
	strcpy(this->id, id);
	this->setLabel(label);
	strcpy(this->type, type);
	strcpy(this->caps, dircaps);
}

uint8_t OPDI_Port::doWork() {
	return OPDI_STATUS_OK;
}

void OPDI_Port::setLabel(const char *label) {
	if (this->label != NULL)
		free(this->label);
	this->label = (char*)malloc(strlen(label) + 1);
	strcpy(this->label, label);
}

uint8_t OPDI_Port::refresh() {
	OPDI_Port **ports = new OPDI_Port*[2];
	ports[0] = this;
	ports[1] = NULL;

	return opdi->refresh(ports);
}

OPDI_Port::~OPDI_Port() {
	if (this->id != NULL)
		free(this->id);
	if (this->label != NULL)
		free(this->label);
	if (this->data != NULL)
		free(this->data);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_DigitalPort::OPDI_DigitalPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, label, OPDI_PORTTYPE_DIGITAL, dircaps, flags, NULL) {
}

OPDI_DigitalPort::~OPDI_DigitalPort() {
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AnalogPort::OPDI_AnalogPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, label, OPDI_PORTTYPE_ANALOG, dircaps, flags, NULL) {
}

OPDI_AnalogPort::~OPDI_AnalogPort() {
}

#endif		// NO_ANALOG_PORTS


//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_EmulatedDigitalPort::OPDI_EmulatedDigitalPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor; mask unsupported flags
	OPDI_DigitalPort(id, label, dircaps, flags & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) & (OPDI_DIGITAL_PORT_HAS_PULLDN | OPDI_DIGITAL_PORT_PULLDN_ALWAYS)) {

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

OPDI_EmulatedAnalogPort::OPDI_EmulatedAnalogPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_AnalogPort(id, label, dircaps, flags) {

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



#include <string.h>

#include <cstdlib>
#include <string>
#include <sstream>

#include "Poco/Exception.h"

#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"

#include "OPDI.h"
#include "OPDI_Ports.h"

// Strings in this class don't have to be localized.
// They are either misconfiguration errors or responses to master programming errors.
// They should never be displayed to the user of a master if code and configuration are correct.

//////////////////////////////////////////////////////////////////////////////////////////
// General port functionality
//////////////////////////////////////////////////////////////////////////////////////////

OPDI_Port::OPDI_Port(const char *id, const char *type) {
	this->data = NULL;
	this->next = NULL;
	this->id = NULL;
	this->label = NULL;
	this->caps[0] = OPDI_PORTDIRCAP_UNKNOWN[0];
	this->caps[1] = '\0';
	this->opdi = NULL;
	this->flags = 0;
	this->ptr = NULL;

	this->id = (char*)malloc(strlen(id) + 1);
	strcpy(this->id, id);
	strcpy(this->type, type);
}

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
	strcpy(this->type, type);

	this->setLabel(label);
	this->setDirCaps(dircaps);
}

uint8_t OPDI_Port::doWork() {
	// self-refresh specified?
	if (this->selfRefreshTime > 0) {
		// self refresh timer reached?
		if (opdi_get_time_ms() - this->lastSelfRefreshTime > this->selfRefreshTime) {
			this->doSelfRefresh();
			this->lastSelfRefreshTime = opdi_get_time_ms();
		}
	}

	return OPDI_STATUS_OK;
}

const char *OPDI_Port::getID(void) {
	return this->id;
}

const char *OPDI_Port::getLabel(void) {
	return this->label;
}

void OPDI_Port::setLabel(const char *label) {
	if (this->label != NULL)
		free(this->label);
	this->label = NULL;
	if (label == NULL)
		return;
	this->label = (char*)malloc(strlen(label) + 1);
	strcpy(this->label, label);
	// label changed; update internal data
	if (this->opdi != NULL)
		this->opdi->updatePortData(this);
}

void OPDI_Port::setDirCaps(const char *dirCaps) {
	this->caps[0] = dirCaps[0];
	this->caps[1] = '\0';

	// label changed; update internal data
	if (this->opdi != NULL)
		this->opdi->updatePortData(this);
}

void OPDI_Port::setFlags(int32_t flags) {
	this->flags = flags;
}

void OPDI_Port::doAutoRefresh(void) {
	// only while connected
	// this is important because if an auto refresh is issued during startup
	// (because a port state may be changed by the configuration) it may fail
	// if the ports to auto-refresh have not already been added.
	if ((this->opdi == NULL) || (!this->opdi->isConnected()))
		return;

	std::vector<OPDI_Port *> portsToRefresh;
	// go through port list
	for(std::vector<std::string>::iterator it = this->autoRefreshPorts.begin(); it != this->autoRefreshPorts.end(); ++it) {
		OPDI_Port *port = opdi->findPortByID((*it).c_str());
		if (port == NULL)
			throw PortError(std::string("Specified auto-refresh port could not be found while auto-refreshing port ") + this->id + ": " + *it);
		portsToRefresh.push_back(port);
	}
	if (portsToRefresh.size() > 0) {
		// add end token
		portsToRefresh.push_back(NULL);
		opdi->refresh(&portsToRefresh[0]);
	}
}

void OPDI_Port::setAutoRefreshPorts(std::string portList) {
	// split list at blanks
	std::stringstream ss(portList);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "")
			this->autoRefreshPorts.push_back(item);
	}
}

void OPDI_Port::doSelfRefresh(void) {
	this->refresh();
}

void OPDI_Port::setSelfRefreshTime(uint32_t timeInMs) {
	this->selfRefreshTime = timeInMs;
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

OPDI_AbstractDigitalPort::OPDI_AbstractDigitalPort(const char *id) : OPDI_Port(id, OPDI_PORTTYPE_DIGITAL) {}

OPDI_AbstractDigitalPort::OPDI_AbstractDigitalPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, label, OPDI_PORTTYPE_DIGITAL, dircaps, flags, NULL) {
}

OPDI_AbstractDigitalPort::~OPDI_AbstractDigitalPort() {
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AbstractAnalogPort::OPDI_AbstractAnalogPort(const char *id) : OPDI_Port(id, OPDI_PORTTYPE_ANALOG) {}

OPDI_AbstractAnalogPort::OPDI_AbstractAnalogPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_Port(id, label, OPDI_PORTTYPE_ANALOG, dircaps, flags, NULL) {
}

OPDI_AbstractAnalogPort::~OPDI_AbstractAnalogPort() {
}

#endif		// NO_ANALOG_PORTS


//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_DigitalPort::OPDI_DigitalPort(const char *id) : OPDI_AbstractDigitalPort(id) {
	this->mode = 0;
	this->line = 0;
	this->setDirCaps(OPDI_PORTDIRCAP_UNKNOWN);
}

OPDI_DigitalPort::OPDI_DigitalPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor; mask unsupported flags (?)
	OPDI_AbstractDigitalPort(id, label, dircaps, flags) { // & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) & (OPDI_DIGITAL_PORT_HAS_PULLDN | OPDI_DIGITAL_PORT_PULLDN_ALWAYS)) 

	this->mode = 0;
	this->line = 0;
	this->setDirCaps(dircaps);
}

OPDI_DigitalPort::~OPDI_DigitalPort() {
}

void OPDI_DigitalPort::doSelfRefresh(void) {
	// only in input modes
	if ((this->mode == OPDI_DIGITAL_MODE_INPUT_FLOATING)
		|| (this->mode == OPDI_DIGITAL_MODE_INPUT_PULLUP)
		|| (this->mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN))
		// call base method
		OPDI_Port::doSelfRefresh();
}

void OPDI_DigitalPort::setDirCaps(const char *dirCaps) {
	OPDI_Port::setDirCaps(dirCaps);

	if (!strcmp(dirCaps, OPDI_PORTDIRCAP_UNKNOWN))
		return;

	// adjust mode to fit capabilities
	// set mode depending on dircaps and flags
	if (!strcmp(dirCaps, OPDI_PORTDIRCAP_INPUT) || !strcmp(dirCaps, OPDI_PORTDIRCAP_BIDI))  {
		if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) == OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
			mode = 1;
		else
		if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) == OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
			mode = 2;
		else
			mode = 0;
	} else
		// direction is output only
		mode = 3;
}

// function that handles the set direction command (opdi_set_digital_port_mode)
void OPDI_DigitalPort::setMode(uint8_t mode) {
	if (mode > 3)
		throw PortError("Digital port mode not supported: " + this->to_string((int)mode));

	// validate mode
	if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT) || !strcmp(this->caps, OPDI_PORTDIRCAP_BIDI))  {
		switch (mode) {
		case 0: // Input
			// if "Input" is requested, map it to the allowed pullup/pulldown input mode if specified
			if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) == OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
				this->mode = 1;
			else
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) == OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				this->mode = 2;
			else
				this->mode = 0;
			break;
		case 1:
			if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) != OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
				throw PortError("Digital port mode not supported; use mode 'Input with pullup': " + this->to_string((int)mode));
			this->mode = 1;
			break;
		case 2:
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) != OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				throw PortError("Digital port mode not supported; use mode 'Input with pulldown': " + this->to_string((int)mode));
			this->mode = 2;
			break;
		case 3:
			if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT))
				throw PortError("Cannot set input only digital port mode to 'Output'");
			this->mode = 3;
		}
	} else {
		// direction is output only
		if (mode < 3)
			throw PortError("Cannot set output only digital port mode to input");
		this->mode = 3;
	}
	this->doAutoRefresh();
}

// function that handles the set line command (opdi_set_digital_port_line)
void OPDI_DigitalPort::setLine(uint8_t line) {
	if (this->mode != 3)
		throw PortError("Cannot set digital port line in input mode");
	this->line = line;
	this->doAutoRefresh();
}

// function that fills in the current port state
void OPDI_DigitalPort::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;
	*line = this->line;
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AnalogPort::OPDI_AnalogPort(const char *id) : OPDI_AbstractAnalogPort(id) {
	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

OPDI_AnalogPort::OPDI_AnalogPort(const char *id, const char *label, const char *dircaps, const uint8_t flags) :
	// call base constructor
	OPDI_AbstractAnalogPort(id, label, dircaps, flags) {

	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

OPDI_AnalogPort::~OPDI_AnalogPort() {
}

void OPDI_AnalogPort::doSelfRefresh(void) {
	// only in input mode
	if ((this->mode == OPDI_ANALOG_MODE_INPUT))
		// call base method
		OPDI_Port::doSelfRefresh();
}

// function that handles the set direction command (opdi_set_digital_port_mode)
void OPDI_AnalogPort::setMode(uint8_t mode) {
	if (mode > 2)
		throw PortError("Analog port mode not supported: " + this->to_string((int)mode));
	this->mode = mode;
	this->doAutoRefresh();
}

void OPDI_AnalogPort::setResolution(uint8_t resolution) {
	if (resolution < 8 || resolution > 12)
		throw PortError("Analog port resolution not supported; allowed values are 8..12 (bits): " + this->to_string((int)resolution));
	// check whether the resolution is supported
	if (((resolution == 8) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_8) != OPDI_ANALOG_PORT_RESOLUTION_8))
		|| ((resolution == 9) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_9) != OPDI_ANALOG_PORT_RESOLUTION_9))
		|| ((resolution == 10) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_10) != OPDI_ANALOG_PORT_RESOLUTION_10))
		|| ((resolution == 11) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_11) != OPDI_ANALOG_PORT_RESOLUTION_11))
		|| ((resolution == 12) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_12) != OPDI_ANALOG_PORT_RESOLUTION_12)))
		throw PortError("Analog port resolution not supported (port flags): " + this->to_string((int)resolution));
	this->resolution = resolution;
	this->doAutoRefresh();
}

void OPDI_AnalogPort::setReference(uint8_t reference) {
	if (reference > 2)
		throw PortError("Analog port reference not supported: " + this->to_string((int)reference));
	this->reference = reference;
	this->doAutoRefresh();
}

void OPDI_AnalogPort::setValue(int32_t value) {
	// not for inputs
	if (this->mode == 0)
		throw PortError("Cannot set analog value on port configured as input");

	// restrict input to possible values
	this->value = value & ((1 << this->resolution) - 1);
	this->doAutoRefresh();
}

// function that fills in the current port state
void OPDI_AnalogPort::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) {
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
}

#endif		// NO_ANALOG_PORTS


#ifndef OPDI_NO_SELECT_PORTS

OPDI_SelectPort::OPDI_SelectPort(const char *id) : OPDI_Port(id, NULL, OPDI_PORTTYPE_SELECT, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	this->items = NULL;
	this->position = 0;
}

OPDI_SelectPort::OPDI_SelectPort(const char *id, const char *label, const char *items[]) 
	: OPDI_Port(id, label, OPDI_PORTTYPE_SELECT, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	this->setItems(items);
	this->position = 0;
}

OPDI_SelectPort::~OPDI_SelectPort() {}

void OPDI_SelectPort::doSelfRefresh(void) {
	// disabled
}

void OPDI_SelectPort::freeItems() {
	if (this->items != NULL) {
		int i = 0;
		const char *item = this->items[i];
		while (item) {
			free((void *)item);
			i++;
			item = this->items[i];
		}
		delete [] this->items;
	}
}

void OPDI_SelectPort::setItems(const char **items) {
	this->freeItems();
	this->items = NULL;
	// determine array size
	const char *item = items[0];
	int itemCount = 0;
	while (item) {
		itemCount++;
		item = items[itemCount];
	}
	// create target array
	this->items = new char*[itemCount + 1];
	// copy strings to array
	item = items[0];
	itemCount = 0;
	while (item) {
		this->items[itemCount] = (char *)malloc(strlen(items[itemCount]) + 1);
		// copy string
		strcpy(this->items[itemCount], items[itemCount]);
		itemCount++;
		item = items[itemCount];
	}
	// end token
	this->items[itemCount] = NULL;
	this->count = itemCount - 1;
}

void OPDI_SelectPort::setPosition(uint16_t position) {
	if (position > count)
		throw PortError("Position must not exceed the number of items: " + this->count);

	this->position = position;
	this->doAutoRefresh();
}

void OPDI_SelectPort::getState(uint16_t *position) {
	*position = this->position;
}

#endif // OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

OPDI_DialPort::OPDI_DialPort(const char *id) : OPDI_Port(id, NULL, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	this->minValue = 0;
	this->maxValue = 0;
	this->step = 0;
	this->position = 0;
}

OPDI_DialPort::OPDI_DialPort(const char *id, const char *label, int32_t minValue, int32_t maxValue, uint32_t step) 
	: OPDI_Port(id, label, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	if (minValue >= maxValue) {
		throw Poco::DataException("Dial port minValue must be < maxValue");
	}
	this->minValue = minValue;
	this->maxValue = maxValue;
	this->step = step;
	this->position = minValue;
}

OPDI_DialPort::~OPDI_DialPort() {}

void OPDI_DialPort::doSelfRefresh(void) {
	// disabled
}

// function that handles position setting; position may be in the range of minValue..maxValue
void OPDI_DialPort::setPosition(int32_t position) {
	if (position < this->minValue)
		throw PortError("Position must not be less than the minimum: " + this->minValue);
	if (position > this->maxValue)
		throw PortError("Position must not be greater than the maximum: " + this->maxValue);
	// correct position to next possible step
	this->position = ((position - this->minValue) / this->step) * this->step + this->minValue;
	this->doAutoRefresh();
}

// function that fills in the current port state
void OPDI_DialPort::getState(int32_t *position) {
	*position = this->position;
}


#endif // OPDI_NO_DIAL_PORTS
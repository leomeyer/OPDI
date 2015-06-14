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
	this->extendedInfo = NULL;
	this->hidden = false;
	this->readonly = false;
	this->refreshMode = REFRESH_NOT_SET;
	this->refreshRequired = false;
	this->refreshTime = 0;
	this->lastRefreshTime = 0;

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
	this->extendedInfo = NULL;
	this->hidden = false;
	this->readonly = false;
	this->refreshMode = REFRESH_NOT_SET;
	this->refreshRequired = false;
	this->refreshTime = 0;
	this->lastRefreshTime = 0;

	this->id = (char*)malloc(strlen(id) + 1);
	strcpy(this->id, id);
	strcpy(this->type, type);

	this->setLabel(label);
	this->setDirCaps(dircaps);
}

uint8_t OPDI_Port::doWork(uint8_t canSend) {

	// while not connected, always reset the flag
	if ((this->opdi == NULL) || !this->opdi->isConnected())
		this->refreshRequired = false;

	// refresh necessary?
	if (canSend && this->refreshRequired) {
		this->refresh();
		this->refreshRequired = false;
	}

	// determine whether periodic self refresh is necessary
	if ((this->refreshMode == REFRESH_PERIODIC) && (this->refreshTime > 0)) {
		// self refresh timer reached?
		if (opdi_get_time_ms() - this->lastRefreshTime > this->refreshTime) {
			this->doSelfRefresh();
			this->lastRefreshTime = opdi_get_time_ms();
		}
	}

	return OPDI_STATUS_OK;
}

const char *OPDI_Port::getID(void) {
	return this->id;
}

std::string OPDI_Port::ID() {
	return std::string(this->getID());
}

const char *OPDI_Port::getType(void) {
	return this->type;
}

const char *OPDI_Port::getLabel(void) {
	return this->label;
}

void OPDI_Port::setHidden(bool hidden) {
	this->hidden = hidden;
}

bool OPDI_Port::isHidden(void) {
	return this->hidden;
}

void OPDI_Port::setReadonly(bool readonly) {
	this->readonly = readonly;
}

bool OPDI_Port::isReadonly(void) {
	return this->readonly;
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
	int32_t oldFlags = this->flags;
	if (this->readonly)
		this->flags = flags | OPDI_PORT_READONLY;
	else
		this->flags = flags;
	// need to update already stored port data?
	if ((this->opdi != NULL) && (oldFlags != this->flags))
		this->opdi->updatePortData(this);
}

void OPDI_Port::updateExtendedInfo(void) {
	std::string exInfo;
	if (this->group.size() > 0) {
		exInfo += "group=" + this->group + ";";
	}
	if (this->unit.size() > 0) {
		exInfo += "unit=" + this->unit + ";";
	}
	if (this->icon.size() > 0) {
		exInfo += "icon=" + this->icon + ";";
	}
	if (this->extendedInfo != NULL) {
		free(this->extendedInfo);
	}
	this->extendedInfo = (char *)malloc(exInfo.size() + 1);
	strcpy(this->extendedInfo, exInfo.c_str());
}

void OPDI_Port::setUnit(std::string unit) {
	if (this->unit != unit) {
		this->unit = unit;
		this->updateExtendedInfo();
		if (this->opdi != NULL)
			this->opdi->updatePortData(this);
	}
}

void OPDI_Port::setIcon(std::string icon) {
	if (this->icon != icon) {
		this->icon = icon;
		this->updateExtendedInfo();
		if (this->opdi != NULL)
			this->opdi->updatePortData(this);
	}
}

void OPDI_Port::setGroup(std::string group) {
	if (this->group != group) {
		this->group = group;
		this->updateExtendedInfo();
		if (this->opdi != NULL)
			this->opdi->updatePortData(this);
	}
}


/*
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
*/

void OPDI_Port::setRefreshMode(RefreshMode refreshMode) {
	this->refreshMode = refreshMode;
}

OPDI_Port::RefreshMode OPDI_Port::getRefreshMode(void) {
	return this->refreshMode;
}

void OPDI_Port::setRefreshTime(uint32_t timeInMs) {
	this->refreshTime = timeInMs;
}

uint8_t OPDI_Port::refresh() {
	if (this->isHidden())
		return OPDI_STATUS_OK;

	OPDI_Port *ports[2];
	ports[0] = this;
	ports[1] = NULL;

	return this->opdi->refresh(ports);
}

void OPDI_Port::prepare() {
	// update flags (for example, OR other flags to current flag settings)
	this->setFlags(this->flags);
}

OPDI_Port::~OPDI_Port() {
	if (this->id != NULL)
		free(this->id);
	if (this->label != NULL)
		free(this->label);
	if (this->data != NULL)
		free(this->data);
}


OPDI_PortGroup::OPDI_PortGroup(const char *id) {
	this->data = NULL;
	this->next = NULL;
	this->id = NULL;
	this->label = NULL;
	this->parent = NULL;
	this->opdi = NULL;
	this->flags = 0;
	this->extendedInfo = NULL;

	this->id = (char*)malloc(strlen(id) + 1);
	strcpy(this->id, id);
	this->label = (char*)malloc(strlen(id) + 1);
	strcpy(this->label, id);
	this->parent = (char*)malloc(1);
	this->parent[0] = '\0';
}

OPDI_PortGroup::~OPDI_PortGroup() {
	if (this->id != NULL)
		free(this->id);
	if (this->label != NULL)
		free(this->label);
	if (this->data != NULL)
		free(this->data);
}

const char *OPDI_PortGroup::getID(void) {
	return this->id;
}

void OPDI_PortGroup::updateExtendedInfo(void) {
	std::string exInfo;
	if (this->icon.size() > 0) {
		exInfo += "icon=" + this->icon + ";";
	}
	if (this->extendedInfo != NULL) {
		free(this->extendedInfo);
	}
	this->extendedInfo = (char *)malloc(exInfo.size() + 1);
	strcpy(this->extendedInfo, exInfo.c_str());
}

void OPDI_PortGroup::setLabel(const char *label) {
	if (this->label != NULL)
		free(this->label);
	this->label = NULL;
	if (label == NULL)
		return;
	this->label = (char*)malloc(strlen(label) + 1);
	strcpy(this->label, label);
	// label changed; update internal data
	if (this->opdi != NULL)
		this->opdi->updatePortGroupData(this);
}

const char *OPDI_PortGroup::getLabel(void) {
	return this->label;
}

void OPDI_PortGroup::setFlags(int32_t flags) {
	int32_t oldFlags = this->flags;
	this->flags = flags;
	// need to update already stored port data?
	if ((this->opdi != NULL) && (oldFlags != this->flags))
		this->opdi->updatePortGroupData(this);
}

void OPDI_PortGroup::setIcon(std::string icon) {
	if (this->icon != icon) {
		this->icon = icon;
		this->updateExtendedInfo();
		if (this->opdi != NULL)
			this->opdi->updatePortGroupData(this);
	}
}

void OPDI_PortGroup::setParent(const char *parent) {
	if (this->parent != NULL)
		free(this->parent);
	this->parent = NULL;
	if (parent == NULL)
		throw Poco::InvalidArgumentException("Parent group ID must never be NULL");
	this->parent = (char*)malloc(strlen(parent) + 1);
	strcpy(this->parent, parent);
	// label changed; update internal data
	if (this->opdi != NULL)
		this->opdi->updatePortGroupData(this);
}

const char *OPDI_PortGroup::getParent(void) {
	return this->parent;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

OPDI_DigitalPort::OPDI_DigitalPort(const char *id) : OPDI_Port(id, OPDI_PORTTYPE_DIGITAL) {
	this->mode = 0;
	this->line = 0;
	this->setDirCaps(OPDI_PORTDIRCAP_BIDI);
}

OPDI_DigitalPort::OPDI_DigitalPort(const char *id, const char *label, const char *dircaps, const int32_t flags) :
	// call base constructor; mask unsupported flags (?)
	OPDI_Port(id, label, OPDI_PORTTYPE_DIGITAL, dircaps, flags, NULL) { // & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) & (OPDI_DIGITAL_PORT_HAS_PULLDN | OPDI_DIGITAL_PORT_PULLDN_ALWAYS)) 

	this->mode = 0;
	this->line = 0;
	this->setDirCaps(dircaps);
}

OPDI_DigitalPort::~OPDI_DigitalPort() {
}

void OPDI_DigitalPort::doSelfRefresh(void) {
	// set flag
	this->refreshRequired = true;
}

void OPDI_DigitalPort::setDirCaps(const char *dirCaps) {
	OPDI_Port::setDirCaps(dirCaps);

	if (!strcmp(dirCaps, OPDI_PORTDIRCAP_UNKNOWN))
		return;

	// adjust mode to fit capabilities
	// set mode depending on dircaps and flags
	if ((dirCaps[0] == OPDI_PORTDIRCAP_INPUT[0]) || (dirCaps[0] == OPDI_PORTDIRCAP_BIDI[0]))  {
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
		throw PortError(std::string(this->getID()) + ": Digital port mode not supported: " + this->to_string((int)mode));

	int8_t newMode = -1;
	// validate mode
	if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT) || !strcmp(this->caps, OPDI_PORTDIRCAP_BIDI))  {
		switch (mode) {
		case 0: // Input
			// if "Input" is requested, map it to the allowed pullup/pulldown input mode if specified
			if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) == OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
				newMode = 1;
			else
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) == OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				newMode = 2;
			else
				newMode = 0;
			break;
		case 1:
			if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) != OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
				throw PortError(std::string(this->getID()) + ": Digital port mode not supported; use mode 'Input with pullup': " + this->to_string((int)mode));
			newMode = 1;
			break;
		case 2:
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) != OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				throw PortError(std::string(this->getID()) + ": Digital port mode not supported; use mode 'Input with pulldown': " + this->to_string((int)mode));
			newMode = 2;
			break;
		case 3:
			if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT))
				throw PortError(std::string(this->getID()) + ": Cannot set input only digital port mode to 'Output'");
			newMode = 3;
		}
	} else {
		// direction is output only
		if (mode < 3)
			throw PortError(std::string(this->getID()) + ": Cannot set output only digital port mode to input");
		newMode = 3;
	}
	if (newMode > -1) {
		if (newMode != this->mode)
			this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
		this->mode = newMode;
	}
}

void OPDI_DigitalPort::setLine(uint8_t line) {
	if (line > 1)
		throw PortError(std::string(this->getID()) + ": Digital port line not supported: " + this->to_string((int)line));
	if (line != this->line)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->line = line;
}

// function that fills in the current port state
void OPDI_DigitalPort::getState(uint8_t *mode, uint8_t *line) {
	*mode = this->mode;
	*line = this->line;
}

bool OPDI_DigitalPort::hasError(void) {
	uint8_t mode;
	uint8_t line;
	try {
		this->getState(&mode, &line);
		return false;
	} catch (...) {
		return true;
	}
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

OPDI_AnalogPort::OPDI_AnalogPort(const char *id) : OPDI_Port(id, OPDI_PORTTYPE_ANALOG) {
	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

OPDI_AnalogPort::OPDI_AnalogPort(const char *id, const char *label, const char *dircaps, const int32_t flags) :
	// call base constructor
	OPDI_Port(id, label, OPDI_PORTTYPE_ANALOG, dircaps, flags, NULL) {

	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

OPDI_AnalogPort::~OPDI_AnalogPort() {
}

void OPDI_AnalogPort::doSelfRefresh(void) {
	this->refreshRequired = true;
}

// function that handles the set direction command (opdi_set_digital_port_mode)
void OPDI_AnalogPort::setMode(uint8_t mode) {
	if (mode > 2)
		throw PortError(std::string(this->getID()) + ": Analog port mode not supported: " + this->to_string((int)mode));
	if (mode != this->mode)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->mode = mode;
}

int32_t OPDI_AnalogPort::validateValue(int32_t value) {
	if (value < 0)
		return 0;
	if (value > (1 << this->resolution) - 1)
		return (1 << this->resolution) - 1;
	return value;
}

void OPDI_AnalogPort::setResolution(uint8_t resolution) {
	if (resolution < 8 || resolution > 12)
		throw PortError(std::string(this->getID()) + ": Analog port resolution not supported; allowed values are 8..12 (bits): " + this->to_string((int)resolution));
	// check whether the resolution is supported
	if (((resolution == 8) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_8) != OPDI_ANALOG_PORT_RESOLUTION_8))
		|| ((resolution == 9) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_9) != OPDI_ANALOG_PORT_RESOLUTION_9))
		|| ((resolution == 10) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_10) != OPDI_ANALOG_PORT_RESOLUTION_10))
		|| ((resolution == 11) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_11) != OPDI_ANALOG_PORT_RESOLUTION_11))
		|| ((resolution == 12) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_12) != OPDI_ANALOG_PORT_RESOLUTION_12)))
		throw PortError(std::string(this->getID()) + ": Analog port resolution not supported (port flags): " + this->to_string((int)resolution));
	if (resolution != this->resolution)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->resolution = resolution;

	if (this->mode != 0)
		this->setValue(this->value);
}

void OPDI_AnalogPort::setReference(uint8_t reference) {
	if (reference > 2)
		throw PortError(std::string(this->getID()) + ": Analog port reference not supported: " + this->to_string((int)reference));
	if (reference != this->reference)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->reference = reference;
}

void OPDI_AnalogPort::setValue(int32_t value) {
	// not for inputs
	if (this->mode == 0)
		throw PortError(std::string(this->getID()) + ": Cannot set analog value on port configured as input");

	// restrict value to possible range
	int32_t newValue = this->validateValue(value);
	if (newValue != this->value)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->value = newValue;
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

double OPDI_AnalogPort::getRelativeValue(void) {
	// query current value
	uint8_t mode;
	uint8_t resolution;
	uint8_t reference;
	int32_t value;
	this->getState(&mode, &resolution, &reference, &value);
	if (resolution == 0)
		return 0;
	return value * 1.0 / ((1 << resolution) - 1);
}

void OPDI_AnalogPort::setRelativeValue(double value) {
	this->setValue(static_cast<int32_t>(value * ((1 << this->resolution) - 1)));
}

bool OPDI_AnalogPort::hasError(void) {
	uint8_t mode;
	uint8_t resolution;
	uint8_t reference;
	int32_t value;
	try {
		this->getState(&mode, &resolution, &reference, &value);
		return false;
	} catch (...) {
		return true;
	}
}

#endif		// NO_ANALOG_PORTS


#ifndef OPDI_NO_SELECT_PORTS

OPDI_SelectPort::OPDI_SelectPort(const char *id) : OPDI_Port(id, NULL, OPDI_PORTTYPE_SELECT, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	this->count = 0;
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
	this->refreshRequired = true;
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
	this->count = 0;
	if (items == NULL)
		return;
	// determine array size
	const char *item = items[0];
	int itemCount = 0;
	while (item) {
		itemCount++;
		item = items[itemCount];
	}
	if (itemCount > 65535)
		throw Poco::DataException(std::string(this->getID()) + "Too many select port items: " + to_string(itemCount));
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
		throw PortError(std::string(this->getID()) + ": Position must not exceed the number of items: " + to_string((int)this->count));

	if (position != this->position)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->position = position;
}

void OPDI_SelectPort::getState(uint16_t *position) {
	*position = this->position;
}


uint16_t OPDI_SelectPort::getMaxPosition(void) {
	return this->count;
}

bool OPDI_SelectPort::hasError(void) {
	uint16_t position;
	try {
		this->getState(&position);
		return false;
	} catch (...) {
		return true;
	}
}

#endif // OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

OPDI_DialPort::OPDI_DialPort(const char *id) : OPDI_Port(id, NULL, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT, 0, NULL) {
	this->minValue = 0;
	this->maxValue = 0;
	this->step = 0;
	this->position = 0;
}

OPDI_DialPort::OPDI_DialPort(const char *id, const char *label, int64_t minValue, int64_t maxValue, uint64_t step) 
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
	this->refreshRequired = true;
}

void OPDI_DialPort::setMin(int64_t min) {
	this->minValue = min;
}

void OPDI_DialPort::setMax(int64_t max) {
	this->maxValue = max;
}

void OPDI_DialPort::setStep(uint64_t step) {
	this->step = step;
}

// function that handles position setting; position may be in the range of minValue..maxValue
void OPDI_DialPort::setPosition(int64_t position) {
	if (position < this->minValue)
		throw PortError(std::string(this->getID()) + ": Position must not be less than the minimum: " + to_string(this->minValue));
	if (position > this->maxValue)
		throw PortError(std::string(this->getID()) + ": Position must not be greater than the maximum: " + to_string(this->maxValue));
	// correct position to next possible step
	int64_t newPosition = ((position - this->minValue) / this->step) * this->step + this->minValue;
	if (newPosition != this->position)
		this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
	this->position = position;
}

// function that fills in the current port state
void OPDI_DialPort::getState(int64_t *position) {
	*position = this->position;
}

bool OPDI_DialPort::hasError(void) {
	int64_t position;
	try {
		this->getState(&position);
		return false;
	} catch (...) {
		return true;
	}
}

#endif // OPDI_NO_DIAL_PORTS

///////////////////////////////////////////////////////////////////////////////
// Streaming Port
///////////////////////////////////////////////////////////////////////////////

void OPDI_StreamingPort::doSelfRefresh(void) {}

OPDI_StreamingPort::OPDI_StreamingPort(const char *id) : 
	OPDI_Port(id, OPDI_PORTTYPE_STREAMING) {
}

OPDI_StreamingPort::~OPDI_StreamingPort() {
}


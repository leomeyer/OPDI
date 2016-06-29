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

// Strings in this file don't have to be localized.
// They are either misconfiguration errors or responses to master programming errors.
// They should never be displayed to the user of a master if code and configuration are correct.

namespace opdi {

//////////////////////////////////////////////////////////////////////////////////////////
// General port functionality
//////////////////////////////////////////////////////////////////////////////////////////

Port::Port(const char *id, const char *type) {
	this->data = nullptr;
	this->id = nullptr;
	this->label = nullptr;
	this->caps[0] = OPDI_PORTDIRCAP_UNKNOWN[0];
	this->caps[1] = '\0';
	this->opdi = nullptr;
	this->flags = 0;
	this->ptr = nullptr;
	this->hidden = false;
	this->readonly = false;
	this->refreshMode = RefreshMode::REFRESH_NOT_SET;
	this->refreshRequired = false;
	this->periodicRefreshTime = 0;
	this->lastRefreshTime = 0;
	this->orderID = -1;
	this->persistent = false;
	this->error = Error::VALUE_OK;

	this->setID(id);
	strcpy(this->type, type);
}

Port::Port(const char *id, const char *label, const char *type, const char *dircaps, int32_t flags, void* ptr) : Port(id, type) {
	this->flags = flags;
	this->ptr = ptr;
	this->setLabel(label);
	this->setDirCaps(dircaps);
}

uint8_t Port::doWork(uint8_t /* canSend */) {
	// there's no refresh as long as the port is not a part of a running OPDI instance
	if (this->opdi == nullptr)
		this->refreshRequired = false;

	// refresh necessary? don't refresh too often
	if (this->refreshRequired && (opdi_get_time_ms() - this->lastRefreshTime > 1000)) {
		this->refresh();
		this->refreshRequired = false;
	}

	// determine whether periodic self refresh is necessary
	if ((this->refreshMode == RefreshMode::REFRESH_PERIODIC) && (this->periodicRefreshTime > 0)) {
		// self refresh timer reached?
		if (opdi_get_time_ms() - this->lastRefreshTime > this->periodicRefreshTime) {
			this->doRefresh();
			this->lastRefreshTime = opdi_get_time_ms();
		}
	}

	return OPDI_STATUS_OK;
}

const char *Port::getID(void) const {
	return this->id;
}

void Port::setID(const char* newID) {
	if (this->id != nullptr)
		free(this->id);
	this->id = (char*)malloc(strlen(newID) + 1);
	strcpy(this->id, newID);
}

std::string Port::ID() const {
	return std::string(this->getID());
}

const char *Port::getType(void) const {
	return this->type;
}

const char *Port::getLabel(void) const {
	return this->label;
}

void Port::setHidden(bool hidden) {
	this->hidden = hidden;
}

bool Port::isHidden(void) const {
	return this->hidden;
}

void Port::setReadonly(bool readonly) {
	this->readonly = readonly;
}

bool Port::isReadonly(void) const {
	return this->readonly;
}

void Port::setPersistent(bool persistent) {
	this->persistent = persistent;
}

bool Port::isPersistent(void) const {
	return this->persistent;
}

void Port::setLabel(const char *label) {
	if (this->label != nullptr)
		free(this->label);
	this->label = nullptr;
	if (label == nullptr)
		return;
	this->label = (char*)malloc(strlen(label) + 1);
	strcpy(this->label, label);
	// label changed; update internal data
	if (this->opdi != nullptr)
		this->opdi->updatePortData(this);
}

void Port::setDirCaps(const char *dirCaps) {
	this->caps[0] = dirCaps[0];
	this->caps[1] = '\0';

	// label changed; update internal data
	if (this->opdi != nullptr)
		this->opdi->updatePortData(this);
}

const char* Port::getDirCaps() const {
	return this->caps;
}

void Port::setFlags(int32_t flags) {
	int32_t oldFlags = this->flags;
	if (this->readonly)
		this->flags = flags | OPDI_PORT_READONLY;
	else
		this->flags = flags;
	// need to update already stored port data?
	if ((this->opdi != nullptr) && (oldFlags != this->flags))
		this->opdi->updatePortData(this);
}

int32_t Port::getFlags() const {
	return this->flags;
}

void Port::updateExtendedInfo(void) {
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
	this->extendedInfo = exInfo;
}

void Port::setUnit(const std::string& unit) {
	if (this->unit != unit) {
		this->unit = unit;
		this->updateExtendedInfo();
		if (this->opdi != nullptr)
			this->opdi->updatePortData(this);
	}
}

void Port::setIcon(const std::string& icon) {
	if (this->icon != icon) {
		this->icon = icon;
		this->updateExtendedInfo();
		if (this->opdi != nullptr)
			this->opdi->updatePortData(this);
	}
}

void Port::setGroup(const std::string& group) {
	if (this->group != group) {
		this->group = group;
		this->updateExtendedInfo();
		if (this->opdi != nullptr)
			this->opdi->updatePortData(this);
	}
}

void Port::setHistory(uint64_t intervalSeconds, int maxCount, const std::vector<int64_t>& values) {
	this->history = "interval=" + this->to_string(intervalSeconds);
	this->history.append(";maxCount=" + this->to_string(maxCount));
	this->history.append(";values=");
	auto it = values.begin();
	auto ite = values.end();
	while (it != ite) {
		if (it != values.begin())
			this->history.append(",");
		this->history.append(this->to_string(*it));
		++it;
	}
	if (this->refreshMode == RefreshMode::REFRESH_AUTO)
		this->refreshRequired = true;
}

void Port::clearHistory(void) {
	this->history.clear();
	if (this->refreshMode == RefreshMode::REFRESH_AUTO)
		this->refreshRequired = true;
}

std::string Port::getExtendedInfo() const {
	return this->extendedInfo;
}

std::string Port::getExtendedState() const {
	if (this->history.empty())
		return "";
	else
		return "history=" + this->escapeKeyValueText(history);
}

std::string Port::escapeKeyValueText(const std::string& str) const {
	std::string result = str;
	size_t start_pos;
	start_pos = 0;
	while ((start_pos = result.find("\\", start_pos)) != std::string::npos) {
		result.replace(start_pos, 1, "\\\\");
		start_pos += 2;
	}
	start_pos = 0;
	while ((start_pos = result.find("=", start_pos)) != std::string::npos) {
		result.replace(start_pos, 1, "\\=");
		start_pos += 2;
	}
	start_pos = 0;
	while ((start_pos = result.find(";", start_pos)) != std::string::npos) {
		result.replace(start_pos, 1, "\\;");
		start_pos += 2;
	}
	return result;
}

void Port::setRefreshMode(RefreshMode refreshMode) {
	this->refreshMode = refreshMode;
}

Port::RefreshMode Port::getRefreshMode(void) {
	return this->refreshMode;
}

void Port::setPeriodicRefreshTime(uint32_t timeInMs) {
	this->periodicRefreshTime = timeInMs;
}

void Port::doRefresh(void) {
	this->refreshRequired = true;
}

uint8_t Port::refresh() {
	if (this->isHidden())
		return OPDI_STATUS_OK;

	Port *ports[2];
	ports[0] = this;
	ports[1] = nullptr;

	this->lastRefreshTime = opdi_get_time_ms();
	return this->opdi->refresh(ports);
}

void Port::prepare() {
	// update flags (for example, OR other flags to current flag settings)
	this->setFlags(this->flags);
	this->updateExtendedInfo();
}

void Port::checkError() const {
	if (this->error == Error::VALUE_EXPIRED)
		throw ValueExpired();
	if (this->error == Error::VALUE_NOT_AVAILABLE)
		throw ValueUnavailable();
}

void Port::setError(Error error) {
	if (this->error != error)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->error = error;
}

Port::Error Port::getError() const {
	return this->error;
}

Port::~Port() {
	if (this->id != nullptr)
		free(this->id);
	if (this->label != nullptr)
		free(this->label);
	// free internal port memory
	if (this->data != nullptr)
		free(this->data);
}


PortGroup::PortGroup(const char *id) {
	this->data = nullptr;
	this->next = nullptr;
	this->id = nullptr;
	this->label = nullptr;
	this->parent = nullptr;
	this->opdi = nullptr;
	this->flags = 0;
	this->extendedInfo = nullptr;

	this->id = (char*)malloc(strlen(id) + 1);
	strcpy(this->id, id);
	this->label = (char*)malloc(strlen(id) + 1);
	strcpy(this->label, id);
	this->parent = (char*)malloc(1);
	this->parent[0] = '\0';
}

PortGroup::~PortGroup() {
	if (this->id != nullptr)
		free(this->id);
	if (this->label != nullptr)
		free(this->label);
	if (this->parent != nullptr)
		free(this->parent);
	if (this->data != nullptr)
		free(this->data);
}

const char *PortGroup::getID(void) {
	return this->id;
}

void PortGroup::updateExtendedInfo(void) {
	std::string exInfo;
	if (this->icon.size() > 0) {
		exInfo += "icon=" + this->icon + ";";
	}
	if (this->extendedInfo != nullptr) {
		free(this->extendedInfo);
	}
	this->extendedInfo = (char *)malloc(exInfo.size() + 1);
	strcpy(this->extendedInfo, exInfo.c_str());
}

void PortGroup::setLabel(const char *label) {
	if (this->label != nullptr)
		free(this->label);
	this->label = nullptr;
	if (label == nullptr)
		return;
	this->label = (char*)malloc(strlen(label) + 1);
	strcpy(this->label, label);
	// label changed; update internal data
	if (this->opdi != nullptr)
		this->opdi->updatePortGroupData(this);
}

const char *PortGroup::getLabel(void) {
	return this->label;
}

void PortGroup::setFlags(int32_t flags) {
	int32_t oldFlags = this->flags;
	this->flags = flags;
	// need to update already stored port data?
	if ((this->opdi != nullptr) && (oldFlags != this->flags))
		this->opdi->updatePortGroupData(this);
}

void PortGroup::setIcon(const std::string& icon) {
	if (this->icon != icon) {
		this->icon = icon;
		this->updateExtendedInfo();
		if (this->opdi != nullptr)
			this->opdi->updatePortGroupData(this);
	}
}

void PortGroup::setParent(const char *parent) {
	if (this->parent != nullptr)
		free(this->parent);
	this->parent = nullptr;
	if (parent == nullptr)
		throw Poco::InvalidArgumentException("Parent group ID must never be nullptr");
	this->parent = (char*)malloc(strlen(parent) + 1);
	strcpy(this->parent, parent);
	// label changed; update internal data
	if (this->opdi != nullptr)
		this->opdi->updatePortGroupData(this);
}

const char *PortGroup::getParent(void) {
	return this->parent;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

DigitalPort::DigitalPort(const char *id) : Port(id, OPDI_PORTTYPE_DIGITAL) {
	this->mode = 0;
	this->line = 0;
	this->setDirCaps(OPDI_PORTDIRCAP_BIDI);
}

DigitalPort::DigitalPort(const char *id, const char *label, const char *dircaps, const int32_t flags) :
	// call base constructor; mask unsupported flags (?)
	Port(id, label, OPDI_PORTTYPE_DIGITAL, dircaps, flags, nullptr) { // & (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_PULLUP_ALWAYS) & (OPDI_DIGITAL_PORT_HAS_PULLDN | OPDI_DIGITAL_PORT_PULLDN_ALWAYS))

	this->mode = 0;
	this->line = 0;
	this->setDirCaps(dircaps);
}

DigitalPort::~DigitalPort() {
}

void DigitalPort::setDirCaps(const char *dirCaps) {
	Port::setDirCaps(dirCaps);

	if (!strcmp(dirCaps, OPDI_PORTDIRCAP_UNKNOWN))
		return;

	// adjust mode to fit capabilities
	// set mode depending on dircaps and flags
	if ((dirCaps[0] == OPDI_PORTDIRCAP_INPUT[0]) || (dirCaps[0] == OPDI_PORTDIRCAP_BIDI[0])) {
		if ((flags & OPDI_DIGITAL_PORT_PULLUP_ALWAYS) == OPDI_DIGITAL_PORT_PULLUP_ALWAYS)
			mode = 1;
		else
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) == OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				mode = 2;
			else
				mode = 0;
	}
	else
		// direction is output only
		mode = 3;
}

void DigitalPort::setMode(uint8_t mode, ChangeSource /*changeSource*/) {
	if (mode > 3)
		throw PortError(this->ID() + ": Digital port mode not supported: " + this->to_string((int)mode));

	int8_t newMode = -1;
	// validate mode
	if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT) || !strcmp(this->caps, OPDI_PORTDIRCAP_BIDI)) {
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
				throw PortError(this->ID() + ": Digital port mode not supported; use mode 'Input with pullup': " + this->to_string((int)mode));
			newMode = 1;
			break;
		case 2:
			if ((flags & OPDI_DIGITAL_PORT_PULLDN_ALWAYS) != OPDI_DIGITAL_PORT_PULLDN_ALWAYS)
				throw PortError(this->ID() + ": Digital port mode not supported; use mode 'Input with pulldown': " + this->to_string((int)mode));
			newMode = 2;
			break;
		case 3:
			if (!strcmp(this->caps, OPDI_PORTDIRCAP_INPUT))
				throw PortError(this->ID() + ": Cannot set input only digital port mode to 'Output'");
			newMode = 3;
		}
	}
	else {
		// direction is output only
		if (mode < 3)
			throw PortError(this->ID() + ": Cannot set output only digital port mode to input");
		newMode = 3;
	}
	if (newMode > -1) {
		if (newMode != this->mode)
			this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
		this->mode = newMode;
		if (persistent && (this->opdi != nullptr))
			this->opdi->persist(this);
	}
}

void DigitalPort::setLine(uint8_t line, ChangeSource /*changeSource*/) {
	if (line > 1)
		throw PortError(this->ID() + ": Digital port line not supported: " + this->to_string((int)line));
	if (this->error != Error::VALUE_OK)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	if (line != this->line)
		this->refreshRequired |= (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->line = line;
	this->error = Error::VALUE_OK;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

void DigitalPort::getState(uint8_t *mode, uint8_t *line) const {
	this->checkError();

	*mode = this->mode;
	*line = this->line;
}

uint8_t DigitalPort::getMode() {
	return this->mode;
}

bool DigitalPort::hasError(void) const {
	uint8_t mode;
	uint8_t line;
	try {
		this->getState(&mode, &line);
		return false;
	}
	catch (...) {
		return true;
	}
}

#endif		// NO_DIGITAL_PORTS

//////////////////////////////////////////////////////////////////////////////////////////
// Analog port functionality
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_ANALOG_PORTS

AnalogPort::AnalogPort(const char *id) : Port(id, OPDI_PORTTYPE_ANALOG) {
	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

AnalogPort::AnalogPort(const char *id, const char *label, const char *dircaps, const int32_t flags) :
	// call base constructor
	Port(id, label, OPDI_PORTTYPE_ANALOG, dircaps, flags, nullptr) {

	this->mode = 0;
	this->value = 0;
	this->reference = 0;
	this->resolution = 0;
}

AnalogPort::~AnalogPort() {
}

void AnalogPort::setMode(uint8_t mode, ChangeSource /*changeSource*/) {
	if (mode > 2)
		throw PortError(this->ID() + ": Analog port mode not supported: " + this->to_string((int)mode));
	if (mode != this->mode)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->mode = mode;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

int32_t AnalogPort::validateValue(int32_t value) const {
	if (value < 0)
		return 0;
	if (value > (1 << this->resolution) - 1)
		return (1 << this->resolution) - 1;
	return value;
}

void AnalogPort::setResolution(uint8_t resolution, ChangeSource /*changeSource*/) {
	if (resolution < 8 || resolution > 12)
		throw PortError(this->ID() + ": Analog port resolution not supported; allowed values are 8..12 (bits): " + this->to_string((int)resolution));
	// check whether the resolution is supported
	if (((resolution == 8) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_8) != OPDI_ANALOG_PORT_RESOLUTION_8))
		|| ((resolution == 9) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_9) != OPDI_ANALOG_PORT_RESOLUTION_9))
		|| ((resolution == 10) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_10) != OPDI_ANALOG_PORT_RESOLUTION_10))
		|| ((resolution == 11) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_11) != OPDI_ANALOG_PORT_RESOLUTION_11))
		|| ((resolution == 12) && ((this->flags & OPDI_ANALOG_PORT_RESOLUTION_12) != OPDI_ANALOG_PORT_RESOLUTION_12)))
		throw PortError(this->ID() + ": Analog port resolution not supported (port flags): " + this->to_string((int)resolution));
	if (resolution != this->resolution)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->resolution = resolution;

	if (this->mode != 0)
		this->setValue(this->value);
	else
		if (persistent && (this->opdi != nullptr))
			this->opdi->persist(this);
}

void AnalogPort::setReference(uint8_t reference, ChangeSource /*changeSource*/) {
	if (reference > 2)
		throw PortError(this->ID() + ": Analog port reference not supported: " + this->to_string((int)reference));
	if (reference != this->reference)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->reference = reference;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

void AnalogPort::setValue(int32_t value, ChangeSource /*changeSource*/) {
	// restrict value to possible range
	int32_t newValue = this->validateValue(value);
	if (this->error != Error::VALUE_OK)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	if (newValue != this->value)
		this->refreshRequired |= (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->value = newValue;
	this->error = Error::VALUE_OK;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

void AnalogPort::getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) const {
	this->checkError();

	*mode = this->mode;
	*resolution = this->resolution;
	*reference = this->reference;
	*value = this->value;
}

double AnalogPort::getRelativeValue(void) {
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

void AnalogPort::setRelativeValue(double value) {
	this->setValue(static_cast<int32_t>(value * ((1 << this->resolution) - 1)));
}

uint8_t AnalogPort::getMode() {
	return this->mode;
}

bool AnalogPort::hasError(void) const {
	uint8_t mode;
	uint8_t resolution;
	uint8_t reference;
	int32_t value;
	try {
		this->getState(&mode, &resolution, &reference, &value);
		return false;
	}
	catch (...) {
		return true;
	}
}

#endif		// NO_ANALOG_PORTS

#ifndef OPDI_NO_SELECT_PORTS

SelectPort::SelectPort(const char *id) : Port(id, nullptr, OPDI_PORTTYPE_SELECT, OPDI_PORTDIRCAP_OUTPUT, 0, nullptr) {
	this->count = 0;
	this->items = nullptr;
	this->position = 0;
}

SelectPort::SelectPort(const char *id, const char *label, const char *items[])
	: Port(id, label, OPDI_PORTTYPE_SELECT, OPDI_PORTDIRCAP_OUTPUT, 0, nullptr) {
	this->setItems(items);
	this->position = 0;
}

SelectPort::~SelectPort() {
	this->freeItems();
}

void SelectPort::freeItems() {
	if (this->items != nullptr) {
		int i = 0;
		const char *item = this->items[i];
		while (item) {
			free((void *)item);
			i++;
			item = this->items[i];
		}
		delete[] this->items;
	}
}

void SelectPort::setItems(const char **items) {
	this->freeItems();
	this->items = nullptr;
	this->count = 0;
	if (items == nullptr)
		return;
	// determine array size
	const char *item = items[0];
	int itemCount = 0;
	while (item) {
		itemCount++;
		item = items[itemCount];
	}
	if (itemCount > 65535)
		throw Poco::DataException(this->ID() + "Too many select port items: " + to_string(itemCount));
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
	this->items[itemCount] = nullptr;
	this->count = itemCount - 1;
}

void SelectPort::setPosition(uint16_t position, ChangeSource /*changeSource*/) {
	if (position > count)
		throw PortError(this->ID() + ": Position must not exceed the number of items: " + to_string((int)this->count));
	if (this->error != Error::VALUE_OK)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	if (position != this->position)
		this->refreshRequired |= (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->position = position;
	this->error = Error::VALUE_OK;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

void SelectPort::getState(uint16_t *position) const {
	this->checkError();

	*position = this->position;
}

const char *SelectPort::getPositionLabel(uint16_t position) {
	return this->items[position];
}

uint16_t SelectPort::getMaxPosition(void) {
	return this->count;
}

bool SelectPort::hasError(void) const {
	uint16_t position;
	try {
		this->getState(&position);
		return false;
	}
	catch (...) {
		return true;
	}
}

#endif // OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

DialPort::DialPort(const char *id) : Port(id, nullptr, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT, 0, nullptr) {
	this->minValue = 0;
	this->maxValue = 0;
	this->step = 0;
	this->position = 0;
}

DialPort::DialPort(const char *id, const char *label, int64_t minValue, int64_t maxValue, uint64_t step)
	: Port(id, label, OPDI_PORTTYPE_DIAL, OPDI_PORTDIRCAP_OUTPUT, 0, nullptr) {
	if (minValue >= maxValue) {
		throw Poco::DataException("Dial port minValue must be < maxValue");
	}
	this->minValue = minValue;
	this->maxValue = maxValue;
	this->step = step;
	this->position = minValue;
}

DialPort::~DialPort() {
	// release additional data structure memory
	opdi_Port *oPort = (opdi_Port *)this->data;

	if (oPort->info.ptr != nullptr)
		free(oPort->info.ptr);
}

int64_t DialPort::getMin(void) {
	return this->minValue;
}

int64_t DialPort::getMax(void) {
	return this->maxValue;
}

int64_t DialPort::getStep(void) {
	return this->step;
}

void DialPort::setMin(int64_t min) {
	this->minValue = min;
}

void DialPort::setMax(int64_t max) {
	this->maxValue = max;
}

void DialPort::setStep(uint64_t step) {
	this->step = step;
}

void DialPort::setPosition(int64_t position, ChangeSource /*changeSource*/) {
	if (position < this->minValue)
		throw PortError(this->ID() + ": Position must not be less than the minimum: " + to_string(this->minValue));
	if (position > this->maxValue)
		throw PortError(this->ID() + ": Position must not be greater than the maximum: " + to_string(this->maxValue));
	// correct position to next possible step
	int64_t newPosition = ((position - this->minValue) / this->step) * this->step + this->minValue;
	if (this->error != Error::VALUE_OK)
		this->refreshRequired = (this->refreshMode == RefreshMode::REFRESH_AUTO);
	if (newPosition != this->position)
		this->refreshRequired |= (this->refreshMode == RefreshMode::REFRESH_AUTO);
	this->position = position;
	this->error = Error::VALUE_OK;
	if (persistent && (this->opdi != nullptr))
		this->opdi->persist(this);
}

void DialPort::getState(int64_t *position) const {
	this->checkError();

	*position = this->position;
}

bool DialPort::hasError(void) const {
	int64_t position;
	try {
		this->getState(&position);
		return false;
	}
	catch (...) {
		return true;
	}
}

#endif // OPDI_NO_DIAL_PORTS

///////////////////////////////////////////////////////////////////////////////
// Streaming Port
///////////////////////////////////////////////////////////////////////////////

StreamingPort::StreamingPort(const char *id) :
	Port(id, OPDI_PORTTYPE_STREAMING) {
}

StreamingPort::~StreamingPort() {
}

}		// namespace opdi
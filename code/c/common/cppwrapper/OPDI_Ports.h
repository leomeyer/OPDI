#pragma once

#include "opdi_platformtypes.h"

class OPDI_AbstractPort {

	virtual const char *getID(void) = 0;
	virtual const char *getlabel(void) = 0;
	virtual const char *getType(void) = 0;
	virtual const char *getDirCaps(void) = 0;

};

class OPDI;

/** Base class for OPDI port wrappers.
 *
 */
class OPDI_Port {

friend class OPDI;

protected:
	// Protected constructor: This class can't be instantiated directly
	OPDI_Port(const char *id, const char *label, const char *type, const char* dircaps, int32_t flags, void* ptr);

	/** Called regularly by the OPDI system. Enables the port to do work.
	 * Override this in subclasses to implement more complex functionality.
	 * In this method, an implementation may send asynchronous messages to the master.
	 * This includes messages like Resync, Refresh, Debug etc.
	 * Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	 * This will usually signal a device error to the master or cause the master to time out.
	 */
	virtual uint8_t doWork();

	char *id;
	char *label;
	char type[2];	// type constants are one character (see opdi_port.h)
	char caps[2];	// caps constants are one character (port direction constants)
	int32_t flags;
	void* ptr;

	// pointer to OPDI class instance
	OPDI *opdi;

	// OPDI implementation management structure
	void* data;

	// linked list of ports - pointer to next port
	OPDI_Port *next;

public:
	/** Virtual destructor for the port. */
	virtual ~OPDI_Port();

	/** Sets the label of the port. */
	void setLabel(const char *label);

	/** Causes the port to be refreshed by sending a refresh message to a connected master. */
	uint8_t refresh();
};


#ifndef OPDI_NO_DIGITAL_PORTS
/** Defines a general digital port.
 *
 */
class OPDI_DigitalPort : public OPDI_Port {

public:
	// Initialize a digital port. Specify one of the OPDI_PORTDIR_CAPS* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	// Note: OPDI_DIGITAL_PORT_HAS_PULLDN is not supported.
	OPDI_DigitalPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_DigitalPort();

	// pure virtual methods that need to be implemented by subclasses

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: input with pulldown on
	// mode = 3: output
	virtual uint8_t setMode(uint8_t mode) = 0;

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual uint8_t setLine(uint8_t line) = 0;

	// function that fills in the current port state
	virtual uint8_t getState(uint8_t *mode, uint8_t *line) = 0;
};


#endif // OPDI_NO_DIGITAL_PORTS

#ifndef OPDI_NO_ANALOG_PORTS
/** Defines a general analog port.
 *
 */
class OPDI_AnalogPort : public OPDI_Port {

public:
	// Initialize an analog port. Specify one of the OPDI_PORTDIR_CAPS* values for dircaps.
	// Specify one or more of the OPDI_ANALOG_PORT_* values for flags, or'ed together, to specify possible settings.
	OPDI_AnalogPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_AnalogPort();

	// pure virtual methods that need to be implemented by subclasses

	// function that handles the set mode command (opdi_set_analog_port_mode)
	// mode = 1: input
	// mode = 2: output
	virtual uint8_t setMode(uint8_t mode) = 0;

	// function that handles the set resolution command (opdi_set_analog_port_resolution)
	// resolution = (8..12): resolution in bits
	virtual uint8_t setResolution(uint8_t resolution) = 0;

	// function that handles the set reference command (opdi_set_analog_port_reference)
	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual uint8_t setReference(uint8_t reference) = 0;

	// function that handles the set value command (opdi_set_analog_port_value)
	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual uint8_t setValue(int32_t value) = 0;

	// function that fills in the current port state
	virtual uint8_t getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) = 0;
};

#endif // OPDI_NO_ANALOG_PORTS



#ifndef OPDI_NO_DIGITAL_PORTS

/** Defines an emulated digital port.
 */
class OPDI_EmulatedDigitalPort : public OPDI_DigitalPort {

protected:
	uint8_t mode;
	uint8_t line;

public:
	// Initialize a digital port. Specify one of the OPDI_PORTDIRCAPS_* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	// Note: OPDI_DIGITAL_PORT_HAS_PULLDN is not supported.
	OPDI_EmulatedDigitalPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_EmulatedDigitalPort();

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: not supported
	// mode = 3: output
	virtual uint8_t setMode(uint8_t mode);

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual uint8_t setLine(uint8_t line);

	// function that fills in the current port state
	virtual uint8_t getState(uint8_t *mode, uint8_t *line);
};

#endif // OPDI_NO_DIGITAL_PORTS

#ifndef OPDI_NO_ANALOG_PORTS

/** Defines an emulated analog port.
 */
class OPDI_EmulatedAnalogPort : public OPDI_AnalogPort {

protected:
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

public:
	OPDI_EmulatedAnalogPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_EmulatedAnalogPort();

	// mode = 0: input
	// mode = 1: output
	virtual uint8_t setMode(uint8_t mode);

	virtual uint8_t setResolution(uint8_t resolution);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual uint8_t setReference(uint8_t reference);

	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual uint8_t setValue(int32_t value);

	virtual uint8_t getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value);
};

#endif // OPDI_NO_ANALOG_PORTS


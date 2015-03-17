#pragma once

#include <string>
#include <sstream>

#include "Poco/Exception.h"

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"

class OPDI;

/** Base class for OPDI port wrappers.
 *
 */
class OPDI_Port {

friend class AbstractOPDID;
friend class OPDI;

protected:

	// protected constructor - for use by friend classes only
	OPDI_Port(const char *id, const char *type);

	// Protected constructor: This class can't be instantiated directly
	OPDI_Port(const char *id, const char *label, const char *type, const char* dircaps, int32_t flags, void* ptr);

	char *id;
	char *label;
	char type[2];	// type constants are one character (see opdi_port.h)
	char caps[2];	// caps constants are one character (port direction constants)
	int32_t flags;
	void* ptr;

	template <class T> std::string to_string(const T& t);

	/** Called regularly by the OPDI system. Enables the port to do work.
	 * Override this in subclasses to implement more complex functionality.
	 * In this method, an implementation may send asynchronous messages to the master.
	 * This includes messages like Resync, Refresh, Debug etc.
	 * Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	 * This will usually signal a device error to the master or cause the master to time out.
	 */
	virtual uint8_t doWork();

	// pointer to OPDI class instance
	OPDI *opdi;

	// OPDI implementation management structure
	void* data;

	// linked list of ports - pointer to next port
	OPDI_Port *next;

public:

	/** This exception can be used by implementations to indicate an error during a port operation.
	 *  Its message will be transferred to the master. */
	class PortError : public Poco::Exception
	{
	public:
		PortError(std::string message): Poco::Exception(message) {};
	};

	/** This exception can be used by implementations to indicate that a port operation is not allowed.
	 *  Its message will be transferred to the master. */
	class AccessDenied : public Poco::Exception
	{
	public:
		AccessDenied(std::string message): Poco::Exception(message) {};
	};

	/** Pointer for help structures. Not used internally; may be used by the application. */
	void* tag;

	/** Virtual destructor for the port. */
	virtual ~OPDI_Port();

	virtual const char *getID(void);

	/** Sets the label of the port. */
	virtual void setLabel(const char *label);

	virtual const char *getLabel(void);

	/** Sets the direction capabilities of the port. */
	virtual void setDirCaps(const char *dirCaps);

	/** Sets the flags of the port. */
	virtual void setFlags(int32_t flags);

	/** Causes the port to be refreshed by sending a refresh message to a connected master. */
	virtual uint8_t refresh();
};

template <class T> inline std::string OPDI_Port::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Abstract port definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

/** Defines a general digital port.
 *
 */
class OPDI_AbstractDigitalPort : public OPDI_Port {

protected:
	// protected constructor - for use by friend classes only
	OPDI_AbstractDigitalPort(const char *id);

public:
	// Initialize a digital port. Specify one of the OPDI_PORTDIR_CAPS* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	OPDI_AbstractDigitalPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_AbstractDigitalPort();

	// pure virtual methods that need to be implemented by subclasses

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: input with pulldown on
	// mode = 3: output
	virtual void setMode(uint8_t mode) = 0;

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual void setLine(uint8_t line) = 0;

	// function that fills in the current port state
	virtual void getState(uint8_t *mode, uint8_t *line) = 0;
};


#endif // OPDI_NO_DIGITAL_PORTS

#ifndef OPDI_NO_ANALOG_PORTS

/** Defines a general analog port.
 *
 */
class OPDI_AbstractAnalogPort : public OPDI_Port {

friend class AbstractOPDID;
friend class OPDI;

protected:
	OPDI_AbstractAnalogPort(const char *id);

public:
	// Initialize an analog port. Specify one of the OPDI_PORTDIR_CAPS* values for dircaps.
	// Specify one or more of the OPDI_ANALOG_PORT_* values for flags, or'ed together, to specify possible settings.
	OPDI_AbstractAnalogPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_AbstractAnalogPort();

	// pure virtual methods that need to be implemented by subclasses

	// function that handles the set mode command (opdi_set_analog_port_mode)
	// mode = 1: input
	// mode = 2: output
	virtual void setMode(uint8_t mode) = 0;

	// function that handles the set resolution command (opdi_set_analog_port_resolution)
	// resolution = (8..12): resolution in bits
	virtual void setResolution(uint8_t resolution) = 0;

	// function that handles the set reference command (opdi_set_analog_port_reference)
	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual void setReference(uint8_t reference) = 0;

	// function that handles the set value command (opdi_set_analog_port_value)
	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value) = 0;

	// function that fills in the current port state
	virtual void getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value) = 0;
};

#endif // OPDI_NO_ANALOG_PORTS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Concrete port definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef OPDI_NO_DIGITAL_PORTS

/** Defines a digital port.
 */
class OPDI_DigitalPort : public OPDI_AbstractDigitalPort {

friend class AbstractOPDID;
friend class OPDI;

protected:
	uint8_t mode;
	uint8_t line;

	// protected constructor - for use by friend classes only
	OPDI_DigitalPort(const char *id);

public:
	// Initialize a digital port. Specify one of the OPDI_PORTDIRCAPS_* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	OPDI_DigitalPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_DigitalPort();

	virtual void setDirCaps(const char *dirCaps);

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: not supported
	// mode = 3: output
	virtual void setMode(uint8_t mode);

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual void setLine(uint8_t line);

	// function that fills in the current port state
	virtual void getState(uint8_t *mode, uint8_t *line);
};

#endif // OPDI_NO_DIGITAL_PORTS

#ifndef OPDI_NO_ANALOG_PORTS

/** Defines an analog port.
 */
class OPDI_AnalogPort : public OPDI_AbstractAnalogPort {

friend class AbstractOPDID;
friend class OPDI;

protected:
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

	// protected constructor - for use by friend classes only
	OPDI_AnalogPort(const char *id);

public:
	OPDI_AnalogPort(const char *id, const char *label, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_AnalogPort();

	// mode = 0: input
	// mode = 1: output
	virtual void setMode(uint8_t mode);

	virtual void setResolution(uint8_t resolution);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual void setReference(uint8_t reference);

	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value);

	virtual void getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value);
};

#endif // OPDI_NO_ANALOG_PORTS

#ifndef OPDI_NO_SELECT_PORTS

/** Defines a select port.
 *
 */
class OPDI_SelectPort : public OPDI_Port {

friend class AbstractOPDID;
friend class OPDI;

protected:
	char **items;
	uint16_t count;
	uint16_t position;

	// protected constructor - for use by friend classes only
	OPDI_SelectPort(const char *id);

	// frees the internal items memory
	void freeItems();
public:
	// Initialize a select port. The direction of a select port is output only.
	// You have to specify a list of items that are the labels of the different select positions. The last element must be NULL.
	// The items are copied into the privately managed data structure of this class.
	OPDI_SelectPort(const char *id, const char *label, const char **items);
	virtual ~OPDI_SelectPort();

	// Copies the items into the privately managed data structure of this class.
	virtual void setItems(const char **items);

	// function that handles position setting; position may be in the range of 0..(items.length - 1)
	virtual void setPosition(uint16_t position);

	// function that fills in the current port state
	virtual void getState(uint16_t *position);
};

#endif // OPDI_NO_SELECT_PORTS

#ifndef OPDI_NO_DIAL_PORTS

/** Defines a dial port.
 *
 */
class OPDI_DialPort : public OPDI_Port {

friend class AbstractOPDID;
friend class OPDI;

protected:
	int32_t minValue;
	int32_t maxValue;
	uint32_t step;
	uint16_t position;

	// protected constructor - for use by friend classes only
	OPDI_DialPort(const char *id);

public:
	// Initialize a dial port. The direction of a dial port is output only.
	// You have to specify boundary values and a step size.
	OPDI_DialPort(const char *id, const char *label, int32_t minValue, int32_t maxValue, uint32_t step);
	virtual ~OPDI_DialPort();

	// function that handles position setting; position may be in the range of minValue..maxValue
	virtual void setPosition(int32_t position);

	// function that fills in the current port state
	virtual void getState(int32_t *position);
};

#endif // OPDI_NO_DIAL_PORTS


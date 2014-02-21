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

/** Defines the OPDI classes that wrap the functions of the OPDI C implementation
 *  for use in Arduino sketches.
 */

#include <opdi_config.h>
#include <opdi_ports.h>

/** Base class for OPDI port wrappers.
 *
 */
class OPDI_Port {

friend class OPDI;

protected:
	// Protected constructor: This class can't be instantiated directly
	OPDI_Port(const char *id, const char *name, const char *type, const char* dircaps);

	char id[MAX_PORTIDLENGTH];
	char name[MAX_PORTNAMELENGTH];
	char type[2];	// type constants are one character (see opdi_ports.h)
	char caps[2];	// caps constants are one character (port direction constants)

	// OPDI implementation management structure
	struct opdi_Port port;

	// linked list of ports - pointer to next port
	OPDI_Port *next;

public:
	/** Sets the name of the port. Maximum length is defined in MAX_PORTNAMELENGTH. */
	void setName(const char *name);

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
	OPDI_DigitalPort(const char *id, const char *name, const char * dircaps, const uint8_t flags);
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

/** Defines a digital port that is bound to a specified pin.
 *
 */
class OPDI_DigitalPortPin : public OPDI_DigitalPort {

protected:
	uint8_t pin;
	uint8_t mode;
	uint8_t line;

public:
	// Initialize a digital port. Specify one of the OPDI_PORTDIRCAPS_* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	// Note: OPDI_DIGITAL_PORT_HAS_PULLDN is not supported.
	OPDI_DigitalPortPin(const char *id, const char *name, const char * dircaps, const uint8_t flags, const uint8_t pin);
	virtual ~OPDI_DigitalPortPin();

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: not supported
	// mode = 3: output
	uint8_t setMode(uint8_t mode);

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	uint8_t setLine(uint8_t line);

	// function that fills in the current port state
	uint8_t getState(uint8_t *mode, uint8_t *line);
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
	OPDI_AnalogPort(const char *id, const char *name, const char * dircaps, const uint8_t flags);
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

/** Defines an analog port that is bound to a specified pin.
	The following restrictions apply:
	1. Port resolution is fixed; 10 bits (0 - 1023) for input ports, and 8 bits (0 - 255) for output ports.
	2. The internal reference setting maps to the analog reference type DEFAULT (not INTERNAL!). It uses the supply voltage
	   of the board as a reference (5V or 3.3V).
	3. The external reference setting maps to the analog reference type EXTERNAL. It uses the voltage at the AREF pin as a reference.
	4. Analog outputs may output an analog value or a PWM signal depending on the board and pin.
 */
class OPDI_AnalogPortPin : public OPDI_AnalogPort {

protected:
	uint8_t pin;
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

public:
	OPDI_AnalogPortPin(const char *id, const char *name, const char * dircaps, const uint8_t flags, const uint8_t pin);
	virtual ~OPDI_AnalogPortPin();

	// mode = 1: input
	// mode = 2: output
	uint8_t setMode(uint8_t mode);

	// does nothing (resolution is fixed to 10 for inputs and 8 for outputs)
	uint8_t setResolution(uint8_t resolution);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	uint8_t setReference(uint8_t reference);

	// value: an integer value ranging from 0 to 2^resolution - 1
	uint8_t setValue(int32_t value);

	uint8_t getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value);
};

#endif // OPDI_NO_ANALOG_PORTS


//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

class OPDI {
protected:
	// list pointers
	OPDI_Port *first_port;
	OPDI_Port *last_port;

	uint32_t idle_timeout_ms;
	uint32_t last_activity;

	// housekeeping function
	uint8_t (*workFunction)();
public:
	/** Prepares the OPDI class for use.
	 *
	 */
	uint8_t setup(const char *slave_name);

	/** Sets the idle timeout. If the master sends no meaningful messages during this time
	 * the slave sends a disconnect message. If the value is 0 (default), the setting has no effect.
	 */
	void setIdleTimeout(uint32_t idleTimeoutMs);

	/** Sets the encoding which must be a valid Java Charset identifier. Examples: ascii, utf-8, iso8859-1.
	 * The default of this implementation is utf-8.
	 */
	void setEncoding(const char* encoding);

	/** Adds the specified port.
	 * */
	uint8_t addPort(OPDI_Port *port);

	/** Internal function.
	 */
	OPDI_Port *findPort(opdi_Port *port);

	/** Starts the OPDI handshake to accept commands from a master.
	 * Does not use a housekeeping function.
	 */
	uint8_t start();

	/** Starts the OPDI handshake to accept commands from a master.
	 * Passes a pointer to a housekeeping function that needs to perform regular tasks.
	 */
	uint8_t start(uint8_t (*workFunction)());

	/** This function is called while the OPDI slave is connected and waiting for messages.
	 * In the default implementation this function calls the workFunction if canSend is true.
	 * You can override it to perform your own housekeeping in case you need to.
	 * If canSend is 1, the slave may send asynchronous messages to the master.
	 * Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	 * This will usually signal a device error to the master or cause the master to time out.
	 */
	virtual uint8_t waiting(uint8_t canSend);

	/** This function returns 1 if a master is currently connected and 0 otherwise.
	 */
	uint8_t isConnected();

	/** Sends the Disconnect message to the master and stops message processing.
	 */
	uint8_t disconnect();

	/** Causes the Reconfigure message to be sent which prompts the master to re-read the device capabilities.
	 */
	uint8_t reconfigure();

	/** Causes the Refresh message to be sent for the specified ports. The last element must be NULL.
	 *  If the first element is NULL, sends the empty refresh message causing all ports to be
	 *  refreshed.
	 */
	uint8_t refresh(OPDI_Port **ports);

	/** An internal handler which is used to implement the idle timer.
	 */
	virtual uint8_t messageHandled(channel_t channel, const char **parts);
};

extern OPDI Opdi;

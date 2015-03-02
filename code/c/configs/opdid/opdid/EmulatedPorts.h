

#include "OPDI.h"

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
	OPDI_EmulatedDigitalPort(const char *id, const char *name, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_EmulatedDigitalPort();

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

/** Defines an emulated analog port.
 */
class OPDI_EmulatedAnalogPort : public OPDI_AnalogPort {

protected:
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

public:
	OPDI_EmulatedAnalogPort(const char *id, const char *name, const char * dircaps, const uint8_t flags);
	virtual ~OPDI_EmulatedAnalogPort();

	// mode = 0: input
	// mode = 1: output
	uint8_t setMode(uint8_t mode);

	uint8_t setResolution(uint8_t resolution);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	uint8_t setReference(uint8_t reference);

	// value: an integer value ranging from 0 to 2^resolution - 1
	uint8_t setValue(int32_t value);

	uint8_t getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value);
};

#endif // OPDI_NO_ANALOG_PORTS

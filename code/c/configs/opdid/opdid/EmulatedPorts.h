

#include "OPDI.h"

#ifndef OPDI_NO_DIGITAL_PORTS

/** Defines a digital port that is bound to a specified pin.
 *
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
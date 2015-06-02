
#include "OPDI.h"

/** Defines the device specific OPDI implementation.
 *
 */
class ArduinOPDI : public OPDI {

public:
	virtual uint8_t setup(const char *slaveName, uint32_t idleTimeout, int16_t deviceFlags);

	virtual ~ArduinOPDI();

	virtual uint32_t getTimeMs() override;
};

#ifndef OPDI_NO_DIGITAL_PORTS

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

	// mode = 0: input
	// mode = 1: output
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

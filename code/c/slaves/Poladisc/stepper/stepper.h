#ifndef _STEPPER_H___
#define _STEPPER_H___

/************************************************************************/
/*                      AVRTools                                        */
/*                      Stepper motor routines							*/
/*                      Version 0.1                                     */
/************************************************************************/
// Leo Meyer, 12/2009


// This code is for direct control of a stepper motor, such as the IC PBL3777.
// If you are using an L297 or similar, you wouldn't use this code but rather
// control the IC via DIR and CLK directly.

// Currently only full-step and half-step modes are supported. For full-step mode,
// only two port pins are necessary. Half-step mode requires four pins.
// If using a standard Roboternetz 10-pin port connector, the pins for the reference
// voltages must manually be set to High, as this code assumes they are permanently
// connected. The pins SENS1 and SENS2 can be freely used. Also, if pins DIS1 and DIS2
// have been connected and full step mode is used, the pins must be managed manually.
// Data direction configuration of the port pins must be done before using this code.
// A limitation of this code is that a stepper motor must be controlled by the pins
// of exactly one port. It is possible to control two steppers in half-step
// mode with one 8 bit port with the pins freely configurable.
// See the test program for an example how to specify the ports and pins.

// For each stepper motor you have to supply a control structure in RAM,
// of type StepperControl. In this way you can control an arbitrary number of steppers
// only limited by RAM size and port pins. The code includes a simple check to ensure
// that only initialized data structures are used to avoid interference with other IO
// in case of a programming error. This check can be disabled for performance reasons,
// however it is recommended that during development the check remain on.

#include <stdint.h>

// Direction constants
// Note that the effective direction depends on stepper motor coil phasing!
enum StepperDirection {
	STEP_LEFT,
	STEP_RIGHT
};

// Disable the checksum test by setting this flag to 1.
// Do this for performance reasons once the code works correctly.
#define STEPPER_NO_CHECKSUM		0

// 8-byte control structure (7 if no checksum required)
typedef struct {
	int8_t phase;		// highest bit indicates mode; 0 = full, 1 = half
	volatile uint8_t *port;		// volatile to avoid warnings because avr definitions are volatile
	uint8_t ph1;
	uint8_t ph2;
	uint8_t dis1;
	uint8_t dis2;
#if (STEPPER_NO_CHECKSUM != 1)
	uint8_t csum;		// checksum to check proper initialization
#endif
} StepperControl;


// Initializes a stepper motor in full-step mode.
extern void stepper_init_full(StepperControl *ctrl, volatile uint8_t *port, const uint8_t pin_ph1, const uint8_t pin_ph2);

// Initializes a stepper motor in half-step mode.
extern void stepper_init_half(StepperControl *ctrl, volatile uint8_t *port, const uint8_t pin_ph1, const uint8_t pin_ph2, 
								const uint8_t pin_dis1, const uint8_t pin_dis2);

// Make one step in the specified direction (STEP_LEFT or STEP_RIGHT).
// Returns 1 in case of an error (uninitialized data structure), otherwise 0.
extern uint8_t stepper_step(StepperControl *ctrl, enum StepperDirection dir);

#endif

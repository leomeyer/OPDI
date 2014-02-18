
/************************************************************************/
/*                      AVRTools                                        */
/*                      Stepper motor routines							*/
/*                      Version 0.1                                     */
/************************************************************************/
// Leo Meyer, 12/2009

#include "stepper.h"

static inline uint8_t stepper_checksum(StepperControl *ctrl) {
	uint8_t *ptr;
	uint8_t sum = 0;
	ptr = (uint8_t*)ctrl;
	for (uint8_t i = 0; i < 7; i++) {	
		sum += ptr[i] ^ 109;
	}
	return sum;
}

// Initializes a stepper motor in full-step mode.
void stepper_init_full(StepperControl *ctrl, volatile uint8_t *port, const uint8_t pin_ph1, const uint8_t pin_ph2) {
	ctrl->port = port;
	ctrl->ph1 = pin_ph1;
	ctrl->ph2 = pin_ph2;
	ctrl->phase = 0;
#if (STEPPER_NO_CHECKSUM != 1)	
	ctrl->csum = stepper_checksum(ctrl);
#endif
}

// Initializes a stepper motor in half-step mode.
void stepper_init_half(StepperControl *ctrl, volatile uint8_t *port, const uint8_t pin_ph1, const uint8_t pin_ph2, 
						const uint8_t pin_dis1, const uint8_t pin_dis2) {
	ctrl->port = port;
	ctrl->ph1 = pin_ph1;
	ctrl->ph2 = pin_ph2;
	ctrl->dis1 = pin_dis1;
	ctrl->dis2 = pin_dis2;
	ctrl->phase = -1;		// highest bit set indicates half-step
#if (STEPPER_NO_CHECKSUM != 1)
	ctrl->csum = stepper_checksum(ctrl);
#endif
}

// Make one step in the specified direction (STEP_LEFT or STEP_RIGHT)
extern uint8_t stepper_step(StepperControl *ctrl, enum StepperDirection dir) {

#if (STEPPER_NO_CHECKSUM != 1)
	// simple checksum test
	// to check for initialized data structure
	if (stepper_checksum(ctrl) != ctrl->csum)
		return 1;
#endif

	uint8_t halfstep = ctrl->phase < 0;
	ctrl->phase &= 0b01111111;		// mask indicator bit

	// determine new phase
	if (dir == STEP_LEFT) {
		ctrl->phase--;
		if (ctrl->phase < 0) {
			// start over again
			if (halfstep) {
				ctrl->phase = 7;	
			} else {
				ctrl->phase = 3;
			}
		}
	} else {
		// STEP_RIGHT
		ctrl->phase++;
		if ((halfstep && (ctrl->phase > 7)) || (!halfstep && ctrl->phase > 3)) {
			// start over again
			ctrl->phase = 0;
		}
	}
	uint8_t bits = 0x0000;
	//				 ^ DIS2
	//				  ^ DIS1
	//				   ^ PH2
	//					^ PH1
	// truth tables taken from mode diagram in PBL3777 datasheet
	if (halfstep) {
		switch (ctrl->phase) {
			case 0: bits = 0b0111; break;
			case 1: bits = 0b0011; break;
			case 2: bits = 0b1001; break;
			case 3: bits = 0b0001; break;
			case 4: bits = 0b0100; break;
			case 5: bits = 0b0000; break;
			case 6: bits = 0b1010; break;
			case 7: bits = 0b0010; break;
		}
	} else {
		// fullstep
		switch (ctrl->phase) {
			case 0: bits = 0b11; break;
			case 1: bits = 0b01; break;
			case 2: bits = 0b00; break;
			case 3: bits = 0b10; break;
		}
	}
	
	// read port bits
	uint8_t val = *(ctrl->port);
	
	// calculate new state
	if (halfstep) {
		// mask previous state
		val &= ~((1 << ctrl->ph1) | (1 << ctrl->ph2) | (1 << ctrl->dis1) | (1 << ctrl->dis2));

		// set proper bits
		if (bits & 0b1000)
			val |= 1 << ctrl->dis2;
		if (bits & 0b0100)
			val |= 1 << ctrl->dis1;
		if (bits & 0b0010)
			val |= 1 << ctrl->ph2;
		if (bits & 0b0001)
			val |= 1 << ctrl->ph1;
	} else {
		// mask previous state
		val &= ~((1 << ctrl->ph1) | (1 << ctrl->ph2));

		// set proper bits
		if (bits & 0b0010)
			val |= 1 << ctrl->ph2;
		if (bits & 0b0001)
			val |= 1 << ctrl->ph1;
	}
	
	// output
	*(ctrl->port) = val;
	
	// add halfstep indicator bit
	ctrl->phase |= halfstep << 7;
	
#if (STEPPER_NO_CHECKSUM != 1)
	ctrl->csum = stepper_checksum(ctrl);
#endif
	return 0;	// ok
}
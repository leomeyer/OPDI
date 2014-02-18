#ifndef _CONTROLS_H___
#define _CONTROLS_H___

/************************************************************************/
/*                      AVRTools                                        */
/*                      Generic input routines							*/
/*                      for buttons etc.								*/
/*                      Version 0.1                                     */
/* Code License: Unknown (assume GPL)									*/
/************************************************************************/
// Leo Meyer, 11/2009

// Adapted from Peter Danneggers code at:
// http://www.mikrocontroller.net/articles/Entprellung#Komfortroutine_.28C_f.C3.BCr_AVR.29

// Read the state of up to eight switches connected to a single port.
// Switches are expected to close to ground.
// Supports repeat and long keypress detection.
// Switches can be mapped to buttons.
// This code uses the timer0 library for debouncing. Time granularity is 10 ms.

// Define constants for different inputs

enum Button {
	BUTTON_LEFT,
	BUTTON_MIDDLE,
	BUTTON_RIGHT
};

#define BUTTON_LEFT 	0
#define BUTTON_MIDDLE	1
#define BUTTON_RIGHT	2

#define KEY_DDR         DDRC						// port direction register
#define KEY_PORT        PORTC						// port setup register
#define KEY_PIN         PINC						// port input
#define KEY0            (1 << PINC4)				// key bit constants
//#define KEY1            (1 << PINC3)
#define KEY2            (1 << PINC5)

#define ALL_KEYS        (KEY0 | KEY2)				// key bitmask

#define REPEAT_MASK     (0)							// repeat detection for these keys
#define REPEAT_START    50							// after 500ms 
#define REPEAT_NEXT     20							// every 200ms

#define LONG_MASK		(KEY0 | KEY2)				// Detect long press for these keys
#define LONG_START    	100							// after 2 seconds (not greater than 255!)

// The mapping from buttons to keys happens in controls.c.

// Used to bring all controls to their initial state.
// Initializes debouncing using the timer0 library.
extern void controls_init(void);

// Used to check the state of a button.
// A value of 0 means up.
extern uint8_t button_down(enum Button button);

// Used to check whether a button has been pressed long.
extern uint8_t button_long(enum Button button);

// Returns 1 if the button has been pressed and released since the last call
// to this function.
extern uint8_t button_clicked(enum Button button);

// Used to wait until a button goes from state ON to state OFF.
// This corresponds to a button release.
// Returns a value != 0 if the button had been pressed longer than
// the LONG_START threshold.
extern uint8_t button_wait_release(enum Button button);

// Waits until all buttons have been released.
extern void button_wait_release_all(void);

#endif

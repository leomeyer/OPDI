/************************************************************************/
/*                      AVRTools                                        */
/*                      Generic input routines							*/
/*                      for buttons etc.								*/
/*                      Version 0.1                                     */
/* Code License: Unknown (assume GPL)									*/
/************************************************************************/
// Leo Meyer, 11/2009

// Read the state of up to eight switches connected to a single port.
// Switches are expected to close to ground.
// Debouncing is done in software.
// Adapted from Peter Danneggers code at:
// http://www.mikrocontroller.net/articles/Entprellung#Komfortroutine_.28C_f.C3.BCr_AVR.29

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "controls.h"
#include "../timer0/timer0.h"

static bool initialized = 0;
static volatile uint8_t key_state = 0;		// debounced and inverted key state:
											// bit = 1: key pressed
static volatile uint8_t key_press = 0;		// key press detect
 
static volatile uint8_t key_rpt = 0;		// key long press and repeat

static volatile uint8_t key_long = 0;		// key long press
 
#define ATOM ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

// This function implements debouncing by polling
// input pins in the timer interrupt handler.
static void controls_hook(void) {
	static uint8_t ct0, ct1, rpt, lng;
	uint8_t i;
	
	i = key_state ^ ~KEY_PIN;						// key changed ?
	ct0 = ~( ct0 & i );							// reset or count ct0
	ct1 = ct0 ^ (ct1 & i);							// reset or count ct1
	i &= ct0 & ct1;									// count until roll over ?
	key_state ^= i;									// then toggle debounced state
	key_press |= key_state & i;						// 0->1: key press detect

	if( (key_state & REPEAT_MASK) == 0 )			// check repeat function
		rpt = REPEAT_START;							// start delay
	if( --rpt == 0 ){
		rpt = REPEAT_NEXT;							// repeat delay
		key_rpt |= key_state & REPEAT_MASK;
	}

	if( (key_state & LONG_MASK) == 0 )				// long press detection
		lng = LONG_START;							// start delay
	if( --lng == 0 ){
		key_long |= key_state & LONG_MASK;
	}

}


///////////////////////////////////////////////////////////////////
//
// check if a key is down.
//
static uint8_t get_key_down( uint8_t key_mask )
{
	key_mask &= key_press;                          // read key(s)
	return key_mask;
}

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed. Each pressed key is reported
// only once
//
static uint8_t get_key_press( uint8_t key_mask )
{
  ATOM {                                          	// read and clear atomic !
	key_mask &= key_press;                          // read key(s)
	key_press ^= key_mask;                          // clear key(s)
  }
  return key_mask;
}

/*
///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported being pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.
//
static uint8_t get_key_rpt( uint8_t key_mask )
{
  ATOM {                                          // read and clear atomic !
	key_mask &= key_rpt;                            // read key(s)
	key_rpt ^= key_mask;                            // clear key(s)
  }
  return key_mask;
}

*/
///////////////////////////////////////////////////////////////////
//
static uint8_t get_key_short( uint8_t key_mask )
{
	ATOM {
	  key_mask =                                          // read key state and key press atomic !
		get_key_press( ~key_state & key_mask );
		// if keys set, reset long mask
		key_long &= ~key_mask;
	}
	return key_mask;
}

///////////////////////////////////////////////////////////////////
//
static uint8_t get_key_long( uint8_t key_mask )
{
	ATOM {
		key_mask &= key_long;                          // read key(s)	  
		if (key_press & key_mask) {				// keys must still be pressed
			return key_mask;
		}
	}
	return 0;
}

// Used to bring all controls to their initial state
void controls_init(void) {

	// prevent multiple inits
	if (initialized) return;
	initialized = true;

	// configure key port for input
	KEY_DDR &= ~ALL_KEYS;
	// and turn on pull up resistors
	KEY_PORT |= ALL_KEYS;

	// include debouncing function every 10 ms
	timer0_add_hook(&controls_hook, 10);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Modify from here to implement the mapping from generic switches to your buttons

uint8_t button_down(enum Button button) {
	int result;
	switch (button) {
		case BUTTON_LEFT: result = get_key_down(KEY0); break;
//		case BUTTON_MIDDLE: result = get_key_short(KEY1); break;
		case BUTTON_RIGHT: result = get_key_down(KEY2); break;
		default: result = 0; break;
	}
	return result;
}


uint8_t button_clicked(enum Button button) {
	int result;
	switch (button) {
		case BUTTON_LEFT: result = get_key_short(KEY0); break;
//		case BUTTON_MIDDLE: result = get_key_short(KEY1); break;
		case BUTTON_RIGHT: result = get_key_short(KEY2); break;
		default: result = 0; break;
	}
	return result;
}

// Busy waiting until the specified button has been released.
// If the button has been pressed long, returns 1 immediately,
// in this case the caller should wait until the button has been released
// before evaluating further input.
uint8_t button_wait_release(enum Button button) {
	while (1) {
		if (button_long(button)) 
			return 1;	// return immediately
		switch (button) {
			case BUTTON_LEFT: if (!get_key_short(KEY0)) return 0; break;
//			case BUTTON_MIDDLE: if (!get_key_short(KEY1)) return 0; break;
			case BUTTON_RIGHT: if (!get_key_short(KEY2)) return 0; break;
			default: return 0; break;
		}	
	}
}

// Returns whether a key has been pressed long
uint8_t button_long(enum Button button) {
	int result;
	switch (button) {
		case BUTTON_LEFT: result = get_key_long(KEY0); break;
//		case BUTTON_MIDDLE: result = get_key_long(KEY1); break;
		case BUTTON_RIGHT: result = get_key_long(KEY2); break;
		default: result = 0; break;
	}
	return result;
}

// Waits until all keys have been released
void button_wait_release_all(void) {
	while ((key_state & ALL_KEYS) != 0) {
		// dummy
		button_down(0);
	}
}

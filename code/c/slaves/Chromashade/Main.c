//--------------------------------------------------------------------------------------
// Chromashade - ATmega168
//
// Based on Wizslave minimum implementation (Wizmini)
//
// ATTENTION! This configuration is almost the limit for the Atmega168 (stack overflow!)
// Careful when adding new variables or ports!
// 
// Code License: GNU GPL
//--------------------------------------------------------------------------------------
// Leo Meyer, leomeyer@gmx.de, 2011-2014

#define F_CPU 	F_OSC
 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_constants.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"

#include "uart.h"
#include "timer0/timer0.h"
//#include "avrfix/avrfix.h"

//-----------------------------------------------------------------------------

// UART baud rate
#define BAUDRATE	9600

#define RED_LED		1
#define GREEN_LED	2
#define BLUE_LED	4
#define WHITE_LED	8
#define ALL_LEDS	(RED_LED | GREEN_LED | BLUE_LED | WHITE_LED)

#define MODE_STABLE		255
#define MODE_OSCILLATE	1

// Oscillation parameters
// 
#define DEFAULT_SPEED		127
#define DEFAULT_BRIGHTNESS	127

#define TIME_UNIT_IN_TICKS	1000
#define PEAK_LENGTH			16
#define RED_MASK			0b1001101
#define GREEN_MASK			0b0100111
#define BLUE_MASK			0b0011011

const uint8_t pwm_table[256] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,
0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27, 0x29, 0x2B, 0x2C,
0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E, 0x40, 0x43, 0x45, 0x47, 0x4A, 0x4C, 0x4F,
0x51, 0x54, 0x57, 0x59, 0x5C, 0x5F, 0x62, 0x64, 0x67, 0x6A, 0x6D, 0x70, 0x73, 0x76, 0x79, 0x7C,
0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8E, 0x91, 0x94, 0x97, 0x9A, 0x9C, 0x9F, 0xA2, 0xA5, 0xA7, 0xAA,
0xAD, 0xAF, 0xB2, 0xB4, 0xB7, 0xB9, 0xBB, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
0xD0, 0xD2, 0xD3, 0xD5, 0xD7, 0xD8, 0xDA, 0xDB, 0xDD, 0xDE, 0xDF, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5,
0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF1, 0xF2,
0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8,
0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC,
0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,
0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF};

static uint8_t EEMEM eeprom_valid_ee;
static uint8_t EEMEM red_ee;
static uint8_t EEMEM green_ee;
static uint8_t EEMEM blue_ee;
static uint8_t EEMEM white_ee;
static uint8_t EEMEM mode_ee;
static uint8_t EEMEM oscSpeed_ee;
static uint8_t EEMEM oscBrightness_ee;


// define device-specific attributes
const char* opdi_slave_name = "Chromashade";
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char* opdi_encoding = "";
const char* opdi_supported_protocols = "BP";
uint16_t opdi_device_flags = 0;

static long idle_timeout_ms = 120000;
static long last_activity = 0;

volatile uint8_t red_target;
volatile uint8_t green_target;
volatile uint8_t blue_target;
volatile uint8_t white_target;

volatile uint8_t red_val;
volatile uint8_t green_val;
volatile uint8_t blue_val;
volatile uint8_t white_val;

volatile int16_t redPWM;
volatile int16_t greenPWM;
volatile int16_t bluePWM;
volatile int16_t whitePWM;

volatile uint8_t pwm_on;
volatile uint8_t pwm_counter;

volatile uint8_t mode;
volatile uint8_t oscSpeed;
volatile uint8_t oscBrightness;

// oscillation variables
uint16_t tickCounter;		// the fastest ticker, incremented every 1 ms
uint8_t peakCounter;		// counts a full peak; 0 to 127 maps to 0 - 255 pwm target, 128 to 255 maps to 255 - 0 pwm target
uint8_t phaseCounter;		// counts the phases from 0 to 6, decides which LEDs get PWM'ed


/** Receives a byte from the UART and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {

	uint16_t data;
	long ticks = timer0_ticks();

	do {
		data = uart_getc();
	} while (((data >> 8) != 0) && ((timer0_ticks() - ticks) < timeout));
		
	// error free?
	if ((data >> 8) == 0) {
		*byte = (uint8_t)data;
//		uart_bb_putc(*byte);
	} else {
		// TODO check this!
		return OPDI_TIMEOUT;
	}	

	return OPDI_STATUS_OK;
}

/** Sends count bytes to the BT module.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	uint16_t counter = 0;
	
	while (counter < count)
		uart_putc(bytes[counter++]);

	return OPDI_STATUS_OK;
}

char* modePositions[] = {"Stable", "Oscillating", NULL};
char* modePositionsDE[] = {"Stabil", "Oszillierend", NULL};

static struct opdi_Port modePort = 
{ 
	.id = "SPM", 
	.name = "Mode",
	.type = OPDI_PORTTYPE_SELECT,
	.caps = OPDI_PORTDIRCAP_OUTPUT,
	.info.ptr = modePositions
};

static struct opdi_Port redPort = 
{ 
	.id = "APR", 
	.name = "Red",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT
};

static struct opdi_Port greenPort = 
{ 
	.id ="APG", 
	.name = "Green",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT
};

static struct opdi_Port bluePort = 
{ 
	.id = "APB", 
	.name = "Blue",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT
};

static struct opdi_Port whitePort = 
{
	.id = "APW", 
	.name = "White",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT	
};

static struct opdi_Port oscSpeedPort = 
{
	.id = "APOS", 
	.name = "Osc. Speed",
 	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT
};

static struct opdi_Port oscBrightnessPort = 
{
	.id = "ABRI", 
	.name = "Brightness",
 	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT
};

// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	// supports German?
	if (strcmp("de_DE", languages) == 0) {
		// set German texts
		modePort.name = "Modus";
		redPort.name = "Rot";
		greenPort.name = "Grün";
		bluePort.name  = "Blau";
		whitePort.name  = "Weiß";
		oscSpeedPort.name = "Geschwindigkeit";
		oscBrightnessPort.name = "Helligkeit";
		
		modePositions[0] = modePositionsDE[0];
		modePositions[1] = modePositionsDE[1];
	}
	
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	// mode and resolution are the same for all ports
	mode[0] = '1';
	res[0] = '0';
	ref[0] = '0';
	if (!strcmp(port->id, redPort.id)) {
		*value = red_target;
	} else
	if (!strcmp(port->id, greenPort.id)) {
		*value = green_target;
	} else
	if (!strcmp(port->id, bluePort.id)) {
		*value = blue_target;
	} else
	if (!strcmp(port->id, whitePort.id)) {
		*value = white_target;
	} else
	if (!strcmp(port->id, oscSpeedPort.id)) {
		*value = oscSpeed;
	} else
	if (!strcmp(port->id, oscBrightnessPort.id)) {
		*value = oscBrightness;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	if (!strcmp(port->id, redPort.id)) {
		red_target = value;
	} else
	if (!strcmp(port->id, greenPort.id)) {
		green_target = value;
	} else
	if (!strcmp(port->id, bluePort.id)) {
		blue_target = value;
	} else
	if (!strcmp(port->id, whitePort.id)) {
		white_target = value;
	} else
	if (!strcmp(port->id, oscSpeedPort.id)) {
		oscSpeed = value;
	} else
	if (!strcmp(port->id, oscBrightnessPort.id)) {
		oscBrightness = value;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;

	return OPDI_STATUS_OK;
}

uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *position) {
	if (!strcmp(port->id, modePort.id)) {
		if (mode == MODE_OSCILLATE)
			*position = 1;
		else {
			// all other mode values (including illegal ones)
			*position = 0;
			mode = MODE_STABLE;		// defensive
		}
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}


uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	// can't change modes
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]) {
	// can't change resolutions
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]) {
	// can't change references
		
	return OPDI_STATUS_OK;
}

static void add_ports(void) {

	opdi_add_port(&modePort);

	if (mode == MODE_STABLE) {
		opdi_add_port(&redPort);
		opdi_add_port(&greenPort);
		opdi_add_port(&bluePort);
		opdi_add_port(&whitePort);
	} else
	if (mode == MODE_OSCILLATE) {
		opdi_add_port(&oscSpeedPort);
		opdi_add_port(&oscBrightnessPort);
	}
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position) {
	uint8_t new_mode = mode;
	
	if (!strcmp(port->id, modePort.id)) {
		if (position == 0)
			new_mode = MODE_STABLE;
		else
		if (position == 1)
			new_mode = MODE_OSCILLATE;
		
		// change?
		if (mode != new_mode) {
			mode = new_mode;

			// port selection has changed, remove ports
			opdi_clear_ports();

			// add ports depending on mode
			add_ports();
			
			// ask the master to reconfigure
			opdi_reconfigure();

			timer0_delay(100);
		}
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

static uint8_t init_wiz(void) {

	// configure ports
	if (opdi_get_ports() == NULL) {
		add_ports();
	}

	return opdi_message_setup(&io_receive, &io_send, NULL);
}

static uint8_t start_wiz(void) {	
	opdi_Message message;
	uint8_t result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK) {
		return result;
	}
	
	// reset activity timer
	last_activity = timer0_ticks();
	
	// initiate handshake
	result = opdi_start(&message, NULL, NULL);

	return result;
}

uint8_t opdi_debug_msg(const uint8_t *msg, uint8_t direction) {
	return OPDI_STATUS_OK;
}

// This macro provides a "smooth scrolling" effect until the target is reached.
#define APPROACH_TARGET(val, target)	\
		if (val > target) { 			\
			val--;						\
		}								\
		else							\
		if (val < target) {				\
			val++;						\
		}
		
// pwm value calculation
// called every 1 ms
void apply_mode(long ticks)
{
	uint8_t target_pwm = 0;
	
	if (mode == MODE_OSCILLATE) {

		// increment ticker
		tickCounter++;
		
		// tick count reached, depending on speed?
		if (tickCounter >= (256 - oscSpeed)) {
			// start counter over
			tickCounter = 0;
			// the peak moves on by one step
			peakCounter++;

			// calculate PWM value
			if (peakCounter <= 127) {
				target_pwm = (peakCounter * oscBrightness) >> 8;
			} else {
				target_pwm = ((255 - peakCounter) * oscBrightness) >> 8;
			}
			
			// a full peak defines a phase
			if (peakCounter == 0) 
				phaseCounter++;
				
			// there are only seven phases
			if (phaseCounter >= 7)
				phaseCounter = 0;

			if ((RED_MASK & (1 << (6 - phaseCounter))) != 0) 
				red_target = target_pwm;
			if ((GREEN_MASK & (1 << (6 - phaseCounter))) != 0) 
				green_target = target_pwm;
			if ((BLUE_MASK & (1 << (6 - phaseCounter))) != 0) 
				blue_target = target_pwm;
				
			// calculate white value, only in a certain range
			target_pwm = 0;
			if ((peakCounter >= 64) && (peakCounter < 196)) {
				if (peakCounter <= 127) {
					target_pwm = ((peakCounter - 64) * 2 * oscBrightness) >> 8;
				} else {
					target_pwm = ((196 - peakCounter) * 2 * oscBrightness) >> 8;
				}
			}
			white_target = target_pwm;
		}

	} else {
		// no oscillation - use unchanged values set by user
	}
	
	APPROACH_TARGET(red_val, red_target)
	APPROACH_TARGET(green_val, green_target)
	APPROACH_TARGET(blue_val, blue_target)
	APPROACH_TARGET(white_val, white_target)
	
	redPWM = pgm_read_byte(&pwm_table[red_val]);
	greenPWM = pgm_read_byte(&pwm_table[green_val]);
	bluePWM = pgm_read_byte(&pwm_table[blue_val]);
	whitePWM = pgm_read_byte(&pwm_table[white_val]);	
}

// Interrupt for software PWM
// MUST be short!
// called with 25 kHz
ISR (TIMER1_COMPA_vect)
{
	uint8_t leds = 0;
	
	if (!pwm_on) return;

	pwm_counter++;		// overflows beyond 255
	if (pwm_counter < redPWM)
		leds |= RED_LED;
	if (pwm_counter < greenPWM)
		leds |= GREEN_LED;
	if (pwm_counter < bluePWM)
		leds |= BLUE_LED;
	if (pwm_counter < whitePWM)
		leds |= WHITE_LED;

	// clear LEDS
	PORTC &= ~ALL_LEDS;
	// set new LED state
	PORTC |= leds;
}
 
void timer1_init(void)
{
	// call ISR TIMER1_COMPA_VECT with 25 kHz

    TIMSK1 = _BV(OCIE1A); // Enable Interrupt Timer/Counter1, Output Compare A (TIMER1_COMPA_vect)
    TCCR1B = _BV(CS11) | _BV(CS10) | _BV(WGM12);    // Clock/64, Mode=CTC
	// ticks/s = 8 MHz / 64 = 125 kHz
    OCR1A = 2; 			// PWM speed: 125 kHz / 5 = 25 kHz
}

uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	if (idle_timeout_ms > 0) {
		if (channel != 0) {
			// reset activity time
			last_activity = timer0_ticks();
		} else {
			// control channel message
			if (timer0_ticks() - last_activity > idle_timeout_ms) {
				opdi_send_debug("Session timeout");
				timer0_delay(100);
				return opdi_disconnect();
			}
		}
	}

	return OPDI_STATUS_OK;
}

/*************************************************************************
// main program
*************************************************************************/
int main(void)
{
	uint8_t result;
	uint8_t i;
	uint8_t mode_save;
	uint8_t red_save;
	uint8_t green_save;
	uint8_t blue_save;
	uint8_t white_save;

	timer0_init();
	// register oscillation hook (once per ms)
	timer0_add_hook(apply_mode, 1);

	DDRC = ALL_LEDS;
	DDRD = (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5);

	// setup uart
	uart_init(UART_BAUD_SELECT(BAUDRATE, F_OSC));
	
	// setup PWM timer interrupt
	timer1_init();
	
	// read EEPROM
	if (eeprom_read_byte(&eeprom_valid_ee) == 1) {
		red_target = eeprom_read_byte(&red_ee);
		green_target = eeprom_read_byte(&green_ee);
		blue_target = eeprom_read_byte(&blue_ee);
		white_target = eeprom_read_byte(&white_ee);
		mode = eeprom_read_byte(&mode_ee);
		oscSpeed = eeprom_read_byte(&oscSpeed_ee);
		oscBrightness = eeprom_read_byte(&oscBrightness_ee);
	}
	else {
		// standard values
		red_target = 0;
		green_target = 0;
		blue_target = 0;
		white_target = 0;
		mode = MODE_OSCILLATE;
		oscSpeed = DEFAULT_SPEED;
		oscBrightness = DEFAULT_BRIGHTNESS;
	}

	// initialize wizslave
	init_wiz();
	
	// no pwm
	pwm_on = 0;

	// enable interrupts
	sei();

	// enable pwm
	pwm_on = 1;	
		
	while (1)                   // main loop
	{
		// data arrived on uart?
		if (uart_has_data()) {
			// start wizslave messaging
			result = start_wiz();
			// show result
			if ((result > 0) && (result != 10)) {	// ignore PROTOCOL_ERROR, may lead to much blinking if master is out of sync
				// normal disconnect or timeout?
				if (result <= 2) {
					// save target values
					mode_save = mode;
					red_save = red_target;
					green_save = green_target;
					blue_save = blue_target;
					white_save = white_target;
					
					// fade out
					mode = MODE_STABLE;
					red_target = 0;
					green_target = 0;
					blue_target = 0;
					white_target = 0;
					
					// wait until all values are 0
					while ((red_val > 0) || (green_val > 0) || (blue_val > 0) || (white_val > 0));
					
					// write EEPROM
					// do this after disconnect, because it
					// 1) saves write cycles
					// 2) avoids flickering
					// as opposed to writing each time the values are changed
					// Small disadvantage: if power is lost before disconnect,
					// changes are forgotten
					cli();
					eeprom_write_byte(&red_ee, red_save);
					eeprom_write_byte(&green_ee, green_save);
					eeprom_write_byte(&blue_ee, blue_save);
					eeprom_write_byte(&white_ee, white_save);
					eeprom_write_byte(&mode_ee, mode_save);
					eeprom_write_byte(&oscSpeed_ee, oscSpeed);
					eeprom_write_byte(&oscBrightness_ee, oscBrightness);
					// validity marker
					eeprom_write_byte(&eeprom_valid_ee, 1);
					sei();		

					// restore targets; fade in
					red_target = red_save;
					green_target = green_save;
					blue_target = blue_save;
					white_target = white_save;

					// wait until all values have reached their original value
					while ((red_val < red_target) || (green_val < green_target) || (blue_val < blue_target) || (white_val < white_target));
					
					// restore mode
					mode = mode_save;
				} else {
					pwm_on = 0;
					PORTC &= ~ALL_LEDS;
					timer0_delay(200);
				
					// signal error
					for (i = 0; i < result; i++) {
						// blink red led
						PORTC ^= RED_LED;
						timer0_delay(200);
						PORTC ^= RED_LED;
						timer0_delay(200);
					}
				
					// start normal pwm again
					pwm_on = 1;
				}
				
			}
		}	
	}	// main loop
	return 0;
}

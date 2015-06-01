//--------------------------------------------------------------------------------------
// Poladisc - ATmega168
//
// Based on Wizslave minimum implementation (Wizmini)
//
// ATTENTION! This configuration is almost the limit for the Atmega168 (stack overflow!)
// Careful when adding new variables or ports!
// 
// Code License: GNU GPL
//--------------------------------------------------------------------------------------
// Leo Meyer, leomeyer@gmx.de, 2011

#define F_CPU 	F_OSC
 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_constants.h"
#include "opdi_message.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "uart.h"
#include "timer0/timer0.h"
#include "stepper/stepper.h"

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
#define MODE_RANDOM		2


static uint8_t EEMEM eeprom_valid_ee;
static uint8_t EEMEM red_ee;
static uint8_t EEMEM green_ee;
static uint8_t EEMEM blue_ee;
static uint8_t EEMEM white_ee;
static uint8_t EEMEM speed_ee;
static uint8_t EEMEM mode_ee;
static uint8_t EEMEM oscRange_ee;
static uint8_t EEMEM oscSpeed_ee;


// define device-specific attributes
const char* opdi_config_name = "Poladisc";
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char* opdi_encoding = "";
const char* opdi_supported_protocols = "BP";
uint16_t opdi_device_flags = 0;

volatile uint8_t red_pwm;
volatile uint8_t green_pwm;
volatile uint8_t blue_pwm;
volatile uint8_t white_pwm;
volatile int8_t red_osc;
volatile int8_t green_osc;
volatile int8_t blue_osc;
volatile int8_t white_osc;
volatile int16_t redval;
volatile int16_t greenval;
volatile int16_t blueval;
volatile int16_t whiteval;
volatile uint8_t pwm_on;
volatile uint8_t pwm_counter;

volatile uint8_t mode;
volatile uint8_t oscRange;	// max oscillation value
volatile uint8_t oscSpeed;
volatile uint8_t oscCounter;	// increment is applied when counter reaches 0
volatile int8_t redIncrement; 	// -1 or +1 (direction setting)
volatile int8_t greenIncrement; 	// -1 or +1 (direction setting)
volatile int8_t blueIncrement; 	// -1 or +1 (direction setting)
volatile int8_t whiteIncrement; 	// -1 or +1 (direction setting)

StepperControl stepper;
volatile int8_t stepper_speed;
volatile uint16_t scaled_stepper_speed;
volatile uint16_t stepper_counter;

#define STEPPER_MAX		32767

#define SET_SPEED(speed)	if (speed == 0) stepper_speed = -127; else stepper_speed = speed - 128; scaled_stepper_speed = 2 * stepper_speed * stepper_speed;

#define INIT_OSC()		red_osc = 0; redIncrement = 1; green_osc = oscRange * 2 / 3; greenIncrement = 1; \
						blue_osc = -oscRange / 3; blueIncrement = -1; white_osc = oscRange; whiteIncrement = -1


/*************************************************************************
Function: uart_bb_putc()
Purpose:  bit banging TX
Input:    byte to be transmitted
Returns:  none
**************************************************************************/
void uart_bb_putc(unsigned char value)
{
#define CR_TAB "\n\t" 
#define STOP_BITS 1 

#define SWUART_PORT PORTD	//TX opdi_Port
#define SWUART_PIN	PD2		//TX Pin

	// implemented via bit-banging (Atmel Note AVR305)
   volatile uint8_t bitcnt = 1+8+STOP_BITS; 
   volatile uint8_t delay = 135;
	
   volatile unsigned char val = ~value; 

   cli(); 

   __asm__ __volatile__( 
      "      sec             ; Start bit           " CR_TAB 

      "0:    brcc 1f              ; If carry set   " CR_TAB 
      "      sbi   %[port],%[pad] ; send a '1'     " CR_TAB 	// logisch 0 (RS232-Pegel)
      "      rjmp 2f              ; else           " CR_TAB 

      "1:    cbi   %[port],%[pad] ; send a '0'     " CR_TAB 	// logisch 1 (RS232-Pegel)
      "      nop"                                    CR_TAB 

      "2:    %~call bit_delay_%=  ; One bit delay  " CR_TAB 
      "      %~call bit_delay_%="                    CR_TAB 

      "      lsr   %[val]   ; Get next bit        " CR_TAB 
      "      dec   %[bitcnt] ; If not all bit sent " CR_TAB 
      "      brne 0b         ; send next           " CR_TAB 
      "      rjmp 5f         ; else: done          " CR_TAB 
                                                     CR_TAB 
      "bit_delay_%=:"                                CR_TAB 
      "      mov   __zero_reg__, %[delay]"           CR_TAB 
      "4:    dec   __zero_reg__"                     CR_TAB 
      "      brne 4b"                                CR_TAB 
      "      ret"                                    CR_TAB 
      "5:"                                           CR_TAB 

      : [bitcnt] "=r" (bitcnt), [val] "=r" (val) 
      : "1" (val), "0" (bitcnt), [delay] "r" (delay), [port] "M" (_SFR_IO_ADDR(SWUART_PORT)), [pad] "M" (SWUART_PIN) 
   ); 

   sei(); 
}



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

char* modePositions[] = {"Stable", "Oscill.", NULL };
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
/*
static struct opdi_Port whitePort = 
{
	.id = "APW", 
	.name = "White",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT	
};
*/
static struct opdi_Port stepperPort = 
{ 
	.id = "APS", 
	.name = "Rotation",
	.type = OPDI_PORTTYPE_ANALOG,
	.caps = OPDI_PORTDIRCAP_OUTPUT	
};

static struct opdi_Port oscRangePort = 
{
	.id = "APOR", 
	.name = "Osc. Range",
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


// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	// no effect

	return OPDI_STATUS_OK;
}

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	// mode and resolution are the same for all ports
	mode[0] = '1';
	res[0] = '0';
	ref[0] = '0';
	if (!strcmp(port->id, redPort.id)) {
		*value = red_pwm;
	} else
	if (!strcmp(port->id, greenPort.id)) {
		*value = green_pwm;
	} else
	if (!strcmp(port->id, bluePort.id)) {
		*value = blue_pwm;
	//} else
	//if (!strcmp(port->id, whitePort.id)) {
	//	*value = white_pwm;
	} else
	if (!strcmp(port->id, stepperPort.id)) {
		*value = stepper_speed + 128;
	} else
	if (!strcmp(port->id, oscRangePort.id)) {
		*value = oscRange;
	} else
	if (!strcmp(port->id, oscSpeedPort.id)) {
		*value = oscSpeed;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	if (!strcmp(port->id, redPort.id)) {
		red_pwm = value;
	} else
	if (!strcmp(port->id, greenPort.id)) {
		green_pwm = value;
	} else
	if (!strcmp(port->id, bluePort.id)) {
		blue_pwm = value;
	//} else
	//if (!strcmp(port->id, whitePort.id)) {
	//	white_pwm = value;
	} else
	if (!strcmp(port->id, stepperPort.id)) {
		SET_SPEED(value);
	} else
	if (!strcmp(port->id, oscRangePort.id)) {
		oscRange = value;
		if (oscRange > 127)
			oscRange = 127;
		INIT_OSC();
	} else
	if (!strcmp(port->id, oscSpeedPort.id)) {
		oscSpeed = value;
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

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t i) {
	if (!strcmp(port->id, modePort.id)) {
		if (i == 1) {
			mode = MODE_OSCILLATE;
			INIT_OSC();
		} else
			mode = MODE_STABLE;		// defensive
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}


static uint8_t init(void) {

	// configure ports
	if (opdi_get_ports() == NULL) {
		opdi_add_port(&modePort);
		opdi_add_port(&redPort);
		opdi_add_port(&greenPort);
		opdi_add_port(&bluePort);
	//	opdi_add_port(&whitePort);
		opdi_add_port(&stepperPort);
		opdi_add_port(&oscRangePort);
		opdi_add_port(&oscSpeedPort);
	}
	
	opdi_slave_init();

	return opdi_message_setup(&io_receive, &io_send, NULL);
}

static uint8_t start(void) {	
	opdi_Message message;
	uint8_t result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK) {
		return result;
	}
	
	// initiate handshake
	result = opdi_slave_start(&message, NULL, NULL);

	return result;
}

uint8_t opdi_debug_msg(const char *msg, uint8_t direction) {
	return OPDI_STATUS_OK;
}

// Interrupt for software PWM
// MUST be short!
ISR (TIMER1_COMPA_vect)
{
	uint8_t ledval = 0;
	
	if (!pwm_on) return;

	// clear LEDS
	PORTC &= ~ALL_LEDS;
	
	pwm_counter++;
	if (pwm_counter < redval)
		ledval |= RED_LED;
	if (pwm_counter < greenval)
		ledval |= GREEN_LED;
	if (pwm_counter < blueval)
		ledval |= BLUE_LED;
	if (pwm_counter < whiteval)
		ledval |= WHITE_LED;
	
	PORTC |= ledval;
	
	// stepper movement
//	if (pwm_counter == 0) {
		stepper_counter -= 10;
		if (stepper_speed > 0) {
			if (stepper_counter <= scaled_stepper_speed) {
				stepper_step(&stepper, STEP_LEFT);
				stepper_counter = STEPPER_MAX;
			}
		} else
		if (stepper_speed < 0) {
			if (stepper_counter <= scaled_stepper_speed) {
				stepper_step(&stepper, STEP_RIGHT);
				stepper_counter = STEPPER_MAX;
			}
		}
//	}
}


#define CALC_PWM(basis, osc)	(((basis > 233 ? 233 : basis) / 2 + osc) * (basis > 233 ? 233 : basis) / 255)

void apply_mode(void)
{
	if (mode == MODE_OSCILLATE) {
		if (oscCounter > 0)
			oscCounter--;
		else {
			// set counter according to speed
			oscCounter = 255 - oscSpeed;

			red_osc += redIncrement;			
			// upper range reached?
			if (red_osc >= oscRange)
				redIncrement = -1;	// count down
			// lower boundary?
			if (red_osc <= -oscRange)
				redIncrement = +1;	// count up

			green_osc += greenIncrement;			
			// upper range reached?
			if (green_osc >= oscRange)
				greenIncrement = -1;	// count down
			// lower boundary?
			if (green_osc <= -oscRange)
				greenIncrement = +1;	// count up

			blue_osc += blueIncrement;			
			// upper range reached?
			if (blue_osc >= oscRange)
				blueIncrement = -1;	// count down
			// lower boundary?
			if (blue_osc <= -oscRange)
				blueIncrement = +1;	// count up

			white_osc += whiteIncrement;			
			// upper range reached?
			if (white_osc >= oscRange)
				whiteIncrement = -1;	// count down
			// lower boundary?
			if (white_osc <= -oscRange)
				whiteIncrement = +1;	// count up
				
			redval = CALC_PWM(red_pwm, red_osc);
//			if (redval > 255) redval = 255;
//			if (redval < 0) redval = 0;
			greenval = CALC_PWM(green_pwm, green_osc);
//			if (greenval > 255) greenval = 255;
//			if (greenval < 0) greenval = 0;
			blueval = CALC_PWM(blue_pwm, blue_osc);
//			if (blueval > 255) blueval = 255;
//			if (blueval < 0) blueval = 0;
			whiteval = CALC_PWM(white_pwm, white_osc);
//			if (whiteval > 255) whiteval = 255;
//			if (whiteval < 0) whiteval = 0;	
		}
	} else {
		// no oscillation
		redval = red_pwm;
		greenval = green_pwm;
		blueval = blue_pwm;
		whiteval = white_pwm;
	}
	
}
 
void timer1_init(void)
{
    TIMSK1 = _BV(OCIE1A); // Enable Interrupt Timer/Counter1, Output Compare A (TIMER1_COMPA_vect)
    TCCR1B = _BV(CS11) | _BV(CS10) | _BV(WGM12);    // Clock/64, Mode=CTC
	// ticks/s = 8 MHz / 64 = 125 kHz
    OCR1A = 5; 			// PWM speed
}


/*************************************************************************
// main program
*************************************************************************/
int main(void)
{
	uint8_t result;
	uint8_t i;

	timer0_init();
	// register oscillation hook (once per ms)
	timer0_add_hook(apply_mode, 1);

	DDRC = ALL_LEDS;
	DDRD = (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5);

	// setup uart
	uart_init(UART_BAUD_SELECT(BAUDRATE, F_OSC));
	
	uart_bb_putc('I');
	uart_bb_putc('N');
	uart_bb_putc('I');
	uart_bb_putc('T');
	uart_bb_putc('\n');	

	// initialize slave
	init();
	
	// setup PWM timer interrupt
	timer1_init();
	
	// initialize stepper motor
	stepper_init_half(&stepper, &PORTD, PD2, PD3, PD4, PD5);

	// read EEPROM
	if (eeprom_read_byte(&eeprom_valid_ee) == 255) {
		// standard values
		red_pwm = 0;
		green_pwm = 0;
		blue_pwm = 30;
		white_pwm = 0;
		SET_SPEED(0);
	} else {
		red_pwm = eeprom_read_byte(&red_ee);
		green_pwm = eeprom_read_byte(&green_ee);
		blue_pwm = eeprom_read_byte(&blue_ee);
		white_pwm = eeprom_read_byte(&white_ee);
		SET_SPEED(eeprom_read_byte(&speed_ee));
		mode = eeprom_read_byte(&mode_ee);
		oscRange = eeprom_read_byte(&oscRange_ee);
		oscSpeed = eeprom_read_byte(&oscSpeed_ee);

		INIT_OSC();
	}

	pwm_on = 0;

	// enable interrupts
	sei();

	// restart blink
	PORTC &= ~ALL_LEDS;
	timer0_delay(200);
	// blink green led
	PORTC ^= GREEN_LED;
	timer0_delay(200);
	PORTC ^= GREEN_LED;
	timer0_delay(200);

	// enable pwm
	pwm_on = 1;	
		
	while (1)                   // main loop
	{
		// data arrived on uart?
		if (uart_has_data()) {
			// start slave messaging
			result = start();
			// show result as blink code
			if ((result > 0) && (result != 10)) {	// ignore PROTOCOL_ERROR, may lead to much blinking if master is out of sync
				pwm_on = 0;
				PORTC &= ~ALL_LEDS;
				timer0_delay(200);
				for (i = 0; i < result; i++) {
					// blink red led
					PORTC ^= RED_LED;
					timer0_delay(200);
					PORTC ^= RED_LED;
					timer0_delay(200);
				}
				
				// write EEPROM
				// do this after disconnect, because it
				// 1) saves write cycles
				// 2) avoids flickering
				// as opposed to writing each time the values are changed
				// Small disadvantage: if power is lost before disconnect,
				// changes are forgotten
				cli();
				eeprom_write_byte(&red_ee, red_pwm);
				eeprom_write_byte(&green_ee, green_pwm);
				eeprom_write_byte(&blue_ee, blue_pwm);
				eeprom_write_byte(&speed_ee, stepper_speed);
				eeprom_write_byte(&mode_ee, mode);
				eeprom_write_byte(&oscRange_ee, oscRange);
				eeprom_write_byte(&oscSpeed_ee, oscSpeed);
				// validity marker
				eeprom_write_byte(&eeprom_valid_ee, 1);
				sei();		
				
				pwm_on = 1;
			}
		}	
	}	// main loop
	return 0;
}

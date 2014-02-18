//--------------------------------------------------------------------------------------
// Wizmini for ATmega8
//
// Wizslave minimum implementation
//
// 
// Code License: GNU GPL
//--------------------------------------------------------------------------------------
// Leo Meyer, leomeyer@gmx.de, 2011

#define F_CPU 	F_OSC
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_constants.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"
#include "uart.h"

#include "timer0/timer0.h"

//-----------------------------------------------------------------------------

// UART baud rate
#define BAUDRATE	9600

// define device-specific attributes
const char* opdi_slave_name = "Wizmini";
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char* opdi_encoding = "";
const char* opdi_supported_protocols = "BP";
uint16_t opdi_device_flags = 0;


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
	if ((data >> 8) == 0)
		*byte = (uint8_t)data;
	// TODO check this!
	else {
		return OPDI_TIMEOUT;
	}

	return OPDI_STATUS_OK;
}

/** Sends count bytes to the socket specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	uart_putc('\n');
	uint16_t counter = 0;
	while (counter < count)
		uart_putc(bytes[counter++]);

	return OPDI_STATUS_OK;
}

static struct opdi_Port digPort1 = { "DP1", "Digital Port" };
static struct opdi_Port anaPort1 = { "AP1", "Analog Port" };

static char dig1mode[] = "1";
static char dig1line[] = "0";

static char ana1mode[] = "1";
static char ana1res[] = "1";
static char ana1ref[] = "0";
static int32_t ana1value = 0;

// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	// no effect

	return OPDI_STATUS_OK;
}

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	if (!strcmp(port->id, anaPort1.id)) {
		mode[0] = ana1mode[0];
		res[0] = ana1res[0];
		ref[0] = ana1ref[0];
		*value = ana1value;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1value = value;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	if (!strcmp(port->id, digPort1.id)) {
		mode[0] = dig1mode[0];
		line[0] = dig1line[0];
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}


uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1mode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1res[0] = res[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1ref[0] = ref[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]) {
	if (!strcmp(port->id, digPort1.id)) {
		dig1line[0] = line[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, digPort1.id)) {
		dig1mode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

static void protocol_callback(uint8_t state) {
}


static uint8_t init_wiz(void) {

	// configure ports
	if (opdi_get_ports() == NULL) {
		digPort1.type = OPDI_PORTTYPE_DIGITAL;
		digPort1.caps = OPDI_PORTDIRCAP_BIDI;
		digPort1.info.i = 1;		// HAS_PULLUP
		opdi_add_port(&digPort1);

		anaPort1.type = OPDI_PORTTYPE_ANALOG;
		anaPort1.caps = OPDI_PORTDIRCAP_OUTPUT;
		opdi_add_port(&anaPort1);
	}

	return opdi_setup(&io_receive, &io_send, NULL);
}

static uint8_t start_wiz(void) {	
	opdi_Message message;
	uint8_t result;
	while (1) {
		
		result = opdi_get_message(&message, OPDI_CANNOT_SEND);
		if (result != OPDI_STATUS_OK) {
			return result;
		}

		// initiate handshake
		result = opdi_start(&message, NULL, &protocol_callback);
		if (result != OPDI_STATUS_OK) {
			return result;
		}
	}
	return result;
}


uint8_t opdi_debug_msg(const uint8_t *msg, uint8_t direction) {
	return OPDI_STATUS_OK;
}

/*************************************************************************
// main program
*************************************************************************/
int main(void)
{

	timer0_init();

	sei();

	// setup uart
	uart_init(BAUDRATE);
	
	// initialize wizslave
	init_wiz();
	
	while (1)                   // main loop
	{
		// data arrived on uart?
		if (uart_has_data()) {
			// start wizslave messaging
			start_wiz();
		}
	
		// do everything else here
	
	}	// main loop
	return 0;
}

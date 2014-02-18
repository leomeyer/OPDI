//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011 Leo Meyer (leo@leomeyer.de)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    

// Implements WizChek specific device functions


#include <string.h>

#include "opdi_device.h"
#include "common.h"


#include "uart.c"

// define device specific attributes
const char* opdi_slave_name = "WizChek BT";
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char* opdi_encoding = "";	// no special encoding
const char* opdi_supported_protocols = "BP";
uint16_t opdi_device_flags = 0;


static uint8_t is_terminated(void) {

	if (button_down(BUTTON_LEFT)) {
		// wait for button release
		button_wait_release_all();
		button_clicked(BUTTON_LEFT);	// clear
		return OPDI_CANCELLED;
	}
	
	return OPDI_STATUS_OK;
}


/** Receives a byte from the UART and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {

	uint16_t data;
	long ticks = timer0_ticks();
	uint8_t terminated = 0;

	// connection closed?
//	if (result == 0)
//		return OPDI_DISCONNECTED;

	do {
		data = uart_getc();
		terminated = is_terminated();
	} while (((data >> 8) != 0) && ((timer0_ticks() - ticks) < timeout) && !terminated);

	// terminated?
	if (terminated)
		return terminated;
		
	// error free?
	if ((data >> 8) == 0)
		*byte = (uint8_t)data;
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


static void show_status(void) {
	// show port status on screen
	char buf[6];
	
	oled_write_string(0, 3, "Dig: ");
	if (dig1mode[0] == '0')
		oled_write_string(5, 3, "IFL");
	else if (dig1mode[0] == '1')
		oled_write_string(5, 3, "IPU");
	else if (dig1mode[0] == '2')
		oled_write_string(5, 3, "IPD");
	else if (dig1mode[0] == '3')
		oled_write_string(5, 3, "OUT");
	oled_write_string(9, 3, (dig1line[0] == '0' ? "LO" : "HI"));
	
	oled_write_string(0, 4, "Ana: ");
	if (ana1mode[0] == '0')
		oled_write_string(5, 4, "IN ");
	else if (ana1mode[0] == '1')
		oled_write_string(5, 4, "OUT");
	fstr_dec(ana1value, 4, 0, '.', buf);
	oled_write_string(8, 4, buf);
}

static void protocol_callback(uint8_t state) {
	if (state == OPDI_PROTOCOL_CONNECTED) {
		// update screen
		show_status();
		oled_write_string(0, 1, opdi_master_name);
	}
}



void oled_clear_line(uint8_t line) {
	oled_write_string(0, line, "             ");
}

uint8_t start(void) {
// connection wait timeout
#define WAIT_TIMEOUT	60000
	opdi_Message message;
	uint8_t result;
	long ticks;
	
	BT_ON;		
	Uart_Init();
	
	// sync serial line for BT module
	result = 0;
	io_send(NULL, &result, 1);

	// setup screen
	oled_color(0x0F);
	oled_clrscr();
		
	info_beep();	

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

	opdi_setup(&io_receive, &io_send, NULL);

	// reset timeout counter
	ticks = timer0_ticks();
	
	while (1) {
		oled_write_string(0, 0, "Waiting...");
		show_status();
		oled_clear_line(1);
		
		result = opdi_get_message(&message, OPDI_CANNOT_SEND);

		if (result == OPDI_TIMEOUT) {
			// no connection after a long time?
			if (timer0_ticks() - ticks > WAIT_TIMEOUT) {
				result = OPDI_CANCELLED;
				break;
			} else
				continue;
		} else if (result != OPDI_STATUS_OK) {
			break;
		}

		oled_write_string(0, 0, "Connected  ");

		// initiate handshake
		result = opdi_start(&message, NULL, &protocol_callback);

		if (result != OPDI_DISCONNECTED){
			break;
		}

		// timeout counter startover after disconnect
		ticks = timer0_ticks();
	}
	
	BT_OFF;
	return result;
}


uint8_t opdi_debug_msg(const uint8_t *msg, uint8_t direction) {
	oled_clear_line(11);
	oled_write_string(0, 11, (const char *)msg);
	return OPDI_STATUS_OK;
}

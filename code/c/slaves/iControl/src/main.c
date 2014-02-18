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

/*
 * Include header files for all drivers that have been imported from
 * AVR Software Framework (ASF).
 */
#include <asf.h>
#include <string.h>
#include <avr/interrupt.h>

#include "opdi_constants.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"

#include "debug.h"
#include "common.h"
#include "avr_compiler.h"
#include "timer0/timer0.h"
#include "format/format.h"
#include "wiz610/wiz610.h"

//-----------------------------------------------------------------------------

/** Sends a debug character to the debug UART. */
static void debug_char(char ch) {
	// send character to debug output
}


/** debug output */
void debug_str(const char *str) {
	while (*str) {
		debug_char(*str);
		str++;
	}
}

/** This function is being used by the OPDI library. */
uint8_t opdi_debug_msg(const uint8_t *str, uint8_t direction) {
	debug_str((const char *)str);
	return OPDI_STATUS_OK;
}

/** Receives a byte from the Wiz610 and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*   Sends streaming messages during waiting for new messages if data is there.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {
	uint8_t result;
	long ticks = timer0_ticks();
	
	do {
		if (canSend) {
			// send streaming data
			result = device_serve_streams();
			if (result != OPDI_STATUS_OK)
				return result;
		}		
	} while (!wiz610_has_data() && ((timer0_ticks() - ticks) < timeout));

	if (wiz610_has_data()) {
		// get received byte
		*byte = wiz610_getc();

		return OPDI_STATUS_OK;		
	}
	
	return OPDI_TIMEOUT;	
}

/** Sends count bytes to the socket specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	uint16_t counter = 0;

	while (counter < count) {
		wiz610_putc(bytes[counter++]);
	}

	return OPDI_STATUS_OK;
}

static uint8_t init_slave(void) {
	uint8_t result;
	
	// initialize the device with its drivers
	result = device_init();	
	if (result != OPDI_STATUS_OK)
		return result;

	return opdi_setup(&io_receive, &io_send, NULL);
}


static void reboot_on_error(char* err) {
	// send error text
	debug_str("ERROR: ");
	debug_str(err);		
	
	// time for uart to send everything
	timer0_delay(200);
	
	// software reset	
	CPU_CCP = CCP_IOREG_gc;
	RST.CTRL = RST_SWRST_bm; 
}

int main (void)
{
	uint8_t result, i;
	
	timer0_init();

	// Enable all interrupt levels
	PMIC.CTRL = 0x07;
	
	// LED pin output
	PORTE.DIRSET = PIN7_bm;
	
	/* Enable global interrupts. */
	sei();
		
	debug_str("\r\nINIT\r\n");

	if (!wiz610_init())
		reboot_on_error("wiz610");

	debug_str("\r\nOK\r\n");

	// initialize slave functions
	if (init_slave() != OPDI_STATUS_OK) {
		reboot_on_error("init slave");
	}
	
	while (1)                   // main loop
	{
		wiz610_close_connection();

		debug_str("\r\nWAITING\r\n");
					
		while (1) {
			// data arrived on uart?
			if (wiz610_has_data()) {
				// start wizslave messaging
				result = device_start();
				// irregular disconnect?
				if (result > 1) {
					for (i = 0; i < result; i++)
						debug_char('E');
					debug_char('\n');				
				}
				break;
			}
			
			// do everything else here
		}
	
	}	// main loop

	return 0;
}

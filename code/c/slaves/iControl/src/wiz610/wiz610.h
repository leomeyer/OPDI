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

// Driver for Wiz610 WLAN module

#ifndef WIZ610_H_
#define WIZ610_H_

#define WIZ610_MESSAGE_MAXLEN	512

// USART Defines
#define WIZ610_USART USARTC0
#define WIZ610_USART_PORT PORTC

#define WIZ610_USART_TX_PIN	PIN3_bm
#define WIZ610_USART_RX_PIN	PIN2_bm

#define WIZ610_USART_RXC_vect USARTC0_RXC_vect
#define WIZ610_USART_DRE_vect USARTC0_DRE_vect

// Set Baudrate to 38400 bps:
// Use the default I/O clock frequency that is 2 MHz.
//
// Baudrate select = (1/(16*(((I/O clock frequency)/Baudrate)-1)
#define WIZ610_BAUDRATE_SELECT	9
#define WIZ610_BAUDRATE_SCALE	-2

// command mode pin for Wiz610
#define WIZ610_PINCONFIGURE		PORTC.DIRSET = PIN4_bm

#define WIZ610_COMMAND_MODE		PORTC.OUTTGL = PIN4_bm
#define WIZ610_DATA_MODE		PORTC.OUTSET = PIN4_bm

// command response timeout (ms)
#define WIZ610_TIMEOUT	10000

/** Returns 1 if the device has been successfully initialized.
*/
uint8_t wiz610_init(void);

/** Returns 1 if data is available.
*/
uint8_t wiz610_has_data(void);

/** Returns the next byte from the device.
*/
uint8_t wiz610_getc(void);

/** Sends the byte to the device.
*/
uint8_t wiz610_putc(uint8_t byte);

/** Closes an active TCP connection on the device.
*/
uint8_t wiz610_close_connection(void);

#endif /* WIZ610_H_ */
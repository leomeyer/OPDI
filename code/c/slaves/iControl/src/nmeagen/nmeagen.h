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

// Generic NMEA driver for serial port GPS


#ifndef NMEAGEN_H_
#define NMEAGEN_H_

#define NMEAGEN_MESSAGE_MAXLEN	512

// USART Defines
#define NMEAGEN_USART USARTE0
#define NMEAGEN_USART_PORT PORTE

#define NMEAGEN_USART_TX_PIN	PIN3_bm
#define NMEAGEN_USART_RX_PIN	PIN2_bm

#define NMEAGEN_USART_RXC_vect USARTE0_RXC_vect
#define NMEAGEN_USART_DRE_vect USARTE0_DRE_vect

// Set Baudrate to 9600 bps:
// Use the default I/O clock frequency that is 2 MHz.
//
// Baudrate select = (1/(16*(((I/O clock frequency)/Baudrate)-1)
#define NMEAGEN_BAUDRATE_SELECT	75
#define NMEAGEN_BAUDRATE_SCALE	-6
	 


/** Initializes the USART.
*/
uint8_t nmeagen_init(void);

/** Polls the USART for data. Returns 1 if a complete message is in the buffer, otherwise 0.
*/
uint8_t nmeagen_has_message(void);

/** Transfers a message to the message buffer (without trailing LF) and returns its pointer.
*/
const char *nmeagen_get_message(void);

/** Sends a command to the NMEA device.
*/
uint8_t nmeagen_send_command(char *command);

#endif /* NMEAGEN_H_ */
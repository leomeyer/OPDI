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

// Driver for CHR-6dm

#ifndef CHR6DM_H_
#define CHR6DM_H_

#define MESSAGE_MAXLEN	512

// USART Defines
#define USART USARTC1
#define USART_PORT PORTC

#define USART_TX_PIN	PIN7_bm
#define USART_RX_PIN	PIN6_bm

#define USART_RXC_vect USARTC1_RXC_vect
#define USART_DRE_vect USARTC1_DRE_vect

// Set Baudrate to 115200 bps:
// Use the default I/O clock frequency that is 2 MHz.
//
// Baudrate select = (1/(16*(((I/O clock frequency)/Baudrate)-1)
#define BAUDRATE_SELECT	11
#define BAUDRATE_SCALE	-7

/** Initializes the USART and the device.
*/
uint8_t chr6dm_init(void);

/** Returns 1 if the device has been detected, 0 otherwise.
*/
uint8_t chr6dm_ready(void);

/** Requests data from the device into the specified buffer which must be large enough.
*   Returns 0 if data is available within a certain timeout period, otherwise returns
*   the number of bytes transferred to the buffer.
*/
uint16_t chr6dm_get_data(uint8_t data[]);

/** Perform the calibration.
*/
uint8_t chr6dm_calibrate(void);

#endif /* CHR6DM_H_ */
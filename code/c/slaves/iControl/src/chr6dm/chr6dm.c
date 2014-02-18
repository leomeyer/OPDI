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

#include "../usart/usart_driver.h"
#include "../timer0/timer0.h"
#include "../avr_compiler.h"

#include "chr6dm.h"

// Time to wait for packet data in ms
#define EXPECT_TIMEOUT	100

// USART data struct used in task
static USART_data_t USART_data;

// ready flag
static uint8_t ready;


////////////////////////////////////////////////////////////////////////////////////

// 2nd level uart abstraction -->
static inline uint8_t uart_has_data(void) {
	return USART_RXBufferData_Available(&USART_data);
}

static inline uint8_t uart_getc(void) {
	return USART_RXBuffer_GetByte(&USART_data);
}

static inline void uart_putc(uint8_t byte) {
	USART_TXBuffer_PutByte(&USART_data, byte);
}
// <-- 2nd level uart abstraction

/** Returns 1 if the characters were returned as expected.
*/
static uint8_t uart_expect(uint8_t *chars) {
	
	int16_t pos = 0;
	uint8_t dta;
	uint8_t ok = 0;
	
	long ticks = timer0_ticks();
	
	while (!ok) {
		// wait until timeout or character received
		while (!uart_has_data() && (timer0_ticks() - ticks < EXPECT_TIMEOUT));
		if (!uart_has_data()) 
			return 0;
		dta = uart_getc();
		// data must match position
		if (dta != chars[pos]) 
			return 0;
		pos++;
		if (chars[pos] == '\0')
			ok = 1;
	}
	timer0_delay(500);
	// clear remaining chars
	while (uart_has_data()) {
		dta = uart_getc();
	}
	// ok
	return 1;
}

/** Sends a command to the device.
*/
static void chr6dm_send_command(uint8_t *command, uint8_t length) {
	uint16_t i = 0;
	uint16_t chksum = 0;
	
	while (i < length) {
		USART_TXBuffer_PutByte(&USART_data, command[i]);
		chksum += (uint16_t)command[i++];
	}		
	// send checksum
	USART_TXBuffer_PutByte(&USART_data, chksum >> 8);
	USART_TXBuffer_PutByte(&USART_data, chksum);
}

// device initialization
uint8_t chr6dm_init()
{
	ready = 0;
	
    // (TX) as output
    USART_PORT.DIRSET   = USART_TX_PIN;
    // (RX) as input
    USART_PORT.DIRCLR   = USART_RX_PIN;
    
    // Use USART and initialize buffers
    USART_InterruptDriver_Initialize(&USART_data, &USART, USART_DREINTLVL_LO_gc);
    
    // USART, 8 Data bits, No Parity, 1 Stop bit
    USART_Format_Set(USART_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, false);
    
    // Enable RXC interrupt
	USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);
    
	USART_Baudrate_Set(&USART, BAUDRATE_SELECT, BAUDRATE_SCALE);

	/* Enable both RX and TX. */
	USART_Rx_Enable(USART_data.usart);
	USART_Tx_Enable(USART_data.usart);

	// Enable PMIC interrupt level low
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
	
	timer0_delay(100);		// give some time
	
	// In broadcast mode (default), the CHR-6dm will flood the RX buffer
	// with data at 200 Hz. It is hard to read consistently from
	// a buffer that is only 256 bytes in size.
	// Therefore we're switching the device to silent mode.
	// In this mode we have to poll for each data packet.
	
	// clear RX buffer
	while (uart_has_data()) uart_getc();
	
	// initialize: set silent mode (as opposed to broadcasting mode)
	chr6dm_send_command((uint8_t *)"snp\x81\0", 5);
	
	timer0_delay(10);		// give some time

	// clear RX buffer again
	while (uart_has_data()) uart_getc();

	// send command again
	chr6dm_send_command((uint8_t *)"snp\x81\0", 5);
	timer0_delay(10);		// give some time

	// expect reply
	if (!uart_expect((uint8_t *)"snp\xb0")) return 0;
	
	ready = 1;
	return 1;
}

uint8_t chr6dm_ready(void) {
	return ready;	
}

/** Fills the buffer with data if available.
*   Returns 0 if there is no data.
*/
uint16_t chr6dm_get_data(uint8_t *buffer) {
	
	uint16_t pos = 0;
	
	chr6dm_send_command((uint8_t *)"snp\x01\0", 5);		// GET_DATA
	timer0_delay(10);		// give some time
	
	// no response?
	if (!uart_has_data()) return 0;

	// fill buffer with bytes
	while (uart_has_data()) {
		buffer[pos++] = uart_getc();
	}
	// data is available
	return pos;
}

uint8_t chr6dm_calibrate(void) {
	return 1;
}

//  Receive complete interrupt service routine.
//  Calls the common receive complete handler with pointer to the correct USART
//  as argument.
ISR( USART_RXC_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{                     
    USART_RXComplete( &USART_data );
}

//  Data register empty  interrupt service routine.
//  Calls the common data register empty complete handler with pointer to the
//  correct USART as argument.
ISR( USART_DRE_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{
    USART_DataRegEmpty( &USART_data );
}

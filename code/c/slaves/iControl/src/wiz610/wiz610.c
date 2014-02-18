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


#include "../usart/usart_driver.h"
#include "../timer0/timer0.h"
#include "../avr_compiler.h"

#include "wiz610.h"

// USART data struct used in task
static USART_data_t USART_data;

////////////////////////////////////////////////////////////////////////////////////

// 2nd level uart abstraction -->
static inline uint8_t uart_has_data(void) {
	return USART_RXBufferData_Available(&USART_data);
}

static inline uint8_t uart_getc(void) {
	return USART_RXBuffer_GetByte(&USART_data);
}

static inline uint8_t uart_putc(uint8_t byte) {
	USART_TXBuffer_PutByte(&USART_data, byte);
	return 1;
}

static uint8_t uart_puts(char *str) {
	uint16_t i = 0;
	while (str[i])
		USART_TXBuffer_PutByte(&USART_data, str[i++]);
	return 1;
}
// <-- 2nd level uart abstraction


/** Returns 1 if the characters were returned as expected.
*/
static uint8_t uart_expect(char *chars) {
	
	int16_t pos = 0;
	uint8_t data;
	uint8_t ok = 0;
	
	long ticks = timer0_ticks();
	
	while (!ok) {
		// wait until timeout or character received
		while (!uart_has_data() && (timer0_ticks() - ticks < WIZ610_TIMEOUT));
		if (!uart_has_data()) 
			return 0;
		data = uart_getc();
		// data must match position
		if (data != chars[pos++]) 
			return 0;
		if (chars[pos] == '\0')
			ok = 1;
	}
	timer0_delay(500);
	// clear remaining chars
	while (uart_has_data()) {
		data = uart_getc();
	}
	// ok
	return 1;
}

uint8_t wiz610_init()
{
	WIZ610_PINCONFIGURE;
	
    // (TX) as output
    WIZ610_USART_PORT.DIRSET   = WIZ610_USART_TX_PIN;
    // (RX) as input
    WIZ610_USART_PORT.DIRCLR   = WIZ610_USART_RX_PIN;
    
    // Use USART and initialize buffers
    USART_InterruptDriver_Initialize(&USART_data, &WIZ610_USART, USART_DREINTLVL_LO_gc);
    
    // USART, 8 Data bits, No Parity, 1 Stop bit
    USART_Format_Set(USART_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, false);
    
    // Enable RXC interrupt
	USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);
    
	USART_Baudrate_Set(&WIZ610_USART, WIZ610_BAUDRATE_SELECT, WIZ610_BAUDRATE_SCALE);

	/* Enable both RX and TX. */
	USART_Rx_Enable(USART_data.usart);
	USART_Tx_Enable(USART_data.usart);

	// Enable PMIC interrupt level low
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
	
	// test for wiz610: Must reply with version string	
	WIZ610_DATA_MODE;
	timer0_delay(100);		// give some time
	WIZ610_COMMAND_MODE;
	timer0_delay(100);		// give some time
	
	// request version string
	uart_puts("<RF>");		
	// expect version request reply
	return uart_expect("<Sv");
}

/** Returns 1 if data is available.
*/
uint8_t wiz610_has_data() {
	return uart_has_data();
}

/** Returns the next byte from the device.
*/
uint8_t wiz610_getc() {
	return uart_getc();
}

/** Sends the byte to the device.
*/
uint8_t wiz610_putc(uint8_t byte) {
	return uart_putc(byte);
}

uint8_t wiz610_close_connection() {
	// terminate the connection
	WIZ610_COMMAND_MODE;
	timer0_delay(100);		// give some time
	uart_puts("<WC>");		// TCP connection close
	timer0_delay(100);		// give some time
	// ignore reply; read characters (important)
	while (uart_has_data()) uart_getc();
	WIZ610_DATA_MODE;
	return 1;
}

//  Receive complete interrupt service routine.
//  Calls the common receive complete handler with pointer to the correct USART
//  as argument.
 
ISR( WIZ610_USART_RXC_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{                     
    USART_RXComplete( &USART_data );
}

//  Data register empty  interrupt service routine.
//  Calls the common data register empty complete handler with pointer to the
//  correct USART as argument.

ISR( WIZ610_USART_DRE_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{
    USART_DataRegEmpty( &USART_data );
}

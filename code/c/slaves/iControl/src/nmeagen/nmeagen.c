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

#include "../usart/usart_driver.h"
#include "../avr_compiler.h"

#include "nmeagen.h"

// USART data struct used in task
static USART_data_t USART_data;

// data buffer
static char data[NMEAGEN_MESSAGE_MAXLEN];

// current position in data buffer
static uint16_t datapos = 0;

// message buffer
static char message[NMEAGEN_MESSAGE_MAXLEN];

////////////////////////////////////////////////////////////////////////////////////

uint8_t nmeagen_init()
{
    // (TX) as output
    NMEAGEN_USART_PORT.DIRSET   = NMEAGEN_USART_TX_PIN;
    // (RX) as input
    NMEAGEN_USART_PORT.DIRCLR   = NMEAGEN_USART_RX_PIN;
    
    // Use USARTx0 and initialize buffers
    USART_InterruptDriver_Initialize(&USART_data, &NMEAGEN_USART, USART_DREINTLVL_LO_gc);
    
    // USARTx0, 8 Data bits, No Parity, 1 Stop bit
    USART_Format_Set(USART_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, false);
    
    // Enable RXC interrupt
	USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);
    
	USART_Baudrate_Set(&NMEAGEN_USART, NMEAGEN_BAUDRATE_SELECT, NMEAGEN_BAUDRATE_SCALE);

	/* Enable both RX and TX. */
	USART_Rx_Enable(USART_data.usart);
	USART_Tx_Enable(USART_data.usart);

	// Enable PMIC interrupt level low
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
	
	return 1;
}

/** Poll USART for data. Returns 1 if a complete message is in the buffer, otherwise 0.
*/
uint8_t nmeagen_has_message() {
    if (!USART_RXBufferData_Available(&USART_data)) 
		return 0;
		
	// transfer data to data buffer
    while (USART_RXBufferData_Available(&USART_data)) {
		// transfer byte
		data[datapos] = USART_RXBuffer_GetByte(&USART_data);
		// EOL?
		if (data[datapos] == '\n') 
			// message complete
			return 1;
		datapos++;
		if (datapos > NMEAGEN_MESSAGE_MAXLEN)
			// rollover
			datapos = 0;
	}
	// no complete message
	return 0;
}

/** Transfers a message to the message buffer (without trailing LF) and returns its pointer.
*/
const char *nmeagen_get_message() {
	uint16_t i;
	for (i = 0; i < datapos; i++) {
		message[i] = data[i];
	}
	// string end
	message[i] = '\0';
	// new message starts at the beginning
	datapos = 0;
	return message;
}

uint8_t nmeagen_send_command(char *command) {
	
	uint16_t i = 0;
	while (command[i])
		USART_TXBuffer_PutByte(&USART_data, command[i++]);
	return 1;
}


//  Receive complete interrupt service routine.
//  Calls the common receive complete handler with pointer to the correct USART
//  as argument.
 
ISR( NMEAGEN_USART_RXC_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{                     
    USART_RXComplete( &USART_data );
}


//  Data register empty  interrupt service routine.
//  Calls the common data register empty complete handler with pointer to the
//  correct USART as argument.

ISR( NMEAGEN_USART_DRE_vect ) // Note that this vector name is a define mapped to the correct interrupt vector
{
    USART_DataRegEmpty( &USART_data );
}

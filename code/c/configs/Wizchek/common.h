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
    
// common.h

#define F_CPU 	F_OSC

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_constants.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"

#include "timer0/timer0.h"
#include "format/format.h"
#include "beep/beep.h"
#include "controls/controls.h"
#include "oled.h"

void info_beep(void);

void err_beep(void);


// Start the device functions
extern uint8_t start(void);

//-----------------------------------------------------------------------------

#define BT_CMD_MODE		nop() /*PORTD &= ~BT_RESET*/
#define BT_UART_MODE	nop() /*PORTD |= BT_RESET*/

#define BT_ON	PORTB |= (1 << PB6)
#define BT_OFF	PORTB &= ~(1 << PB6)


#define SLEEP_TICKS	30000

#define KEY_C_LEFT		(1 << PC5)
#define KEY_C_RIGHT		(1 << PC4)
#define KEY_D_MIDDLE	(1 << PD3)

#define BT_STATUS		(1 << PD6)

#define BAUDRATE	9600
	
#define DISPLAY_WIDTH 	OLED_WIDTH
#define DISPLAY_HEIGHT 	OLED_HEIGHT

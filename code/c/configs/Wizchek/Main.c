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
    
//--------------------------------------------------------------------------------------
// ATmega168PA 8MHz - WizChek BT
//
// Wizslave implementation with the Accu-Chek(tm) Compact Plus
//
// Remove large IC QFP100
//
//       efuse = 0x01
//       hfuse = 0xD7
//       lfuse = 0xE2
//
 
#include <avr/io.h>
#include <avr/sleep.h>
#include <compat/deprecated.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "common.h"

uint8_t restarted = 0;

//-----------------------------------------------------------------------------
// Interruptbehandlung: mittlere Taste
//                      Aufwachen aus PowerDown 
//	Interrupt calibration
//----------------------------------------------------------------------------
ISR (INT1_vect) 
{
	EIMSK &= ~(1 << INT1);     // External Interrupt Request Diable

	// wait until button released
	while (!(PIND & KEY_D_MIDDLE)) {}
};

//-----------------------------------------------------------------------------
// SleepMode einschalten -> Aufwachen mit Int1 (mittlere Taste)
//-----------------------------------------------------------------------------
static void sleep(void)
{
	// switch off Oled 
	oled_off();
	
	BT_OFF;

	EIMSK |= (1 << INT1);                  // externen Interrupt freigeben

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
	
	// switch oled on again
	oled_on();
}

void info_beep(void) {
	sound_on(100);
	timer0_delay(50);
	sound_off();  
}

void err_beep(void) {
	sound_on(200);
	timer0_delay(10);
	sound_off();  
}


/*************************************************************************
// main program
*************************************************************************/
int main(void)
{
	char dest[OLED_WIDTH];
	uint8_t result = 0;
	
	DDRB  = OLED_B_PIN_CS | OLED_B_PIN_LOAD;
	PORTB = 0;
		
	DDRC  = OLED_C_PIN_RST | OLED_C_PIN_DC | (1 << PC3);
	// pullups for keys
	PORTC = KEY_C_LEFT | KEY_C_RIGHT;

	DDRD = OLED_D_PIN_12V | OLED_D_PIN_SCLK | OLED_D_PIN_SDIN;
	// pullups for keys
	PORTD = KEY_D_MIDDLE;

	ACSR = 0x80;
	// Interrupt konfigurieren
 
    MCUCR &= ~0x3;              // levelgesteuerter Interrupt an INT1		

	timer0_init();
	controls_init();

	sei();
	
	oled_on();

	while (1)                   // main loop
	{
		BT_OFF;
	
		oled_color(0x0F);		
		oled_clrscr();
		oled_write_string(0, 0, "Wizchek Slave");
		oled_write_string(0, 1, "on Bluetooth");

		oled_color(0x07);		
		oled_write_string(0, 3, "M key:");
		oled_write_string(2, 4, "Off");
		oled_write_string(0, 5, "Middle key:");
		oled_write_string(2, 6, "Start");
		oled_color(0x0F);		
		if (restarted == 0) {
			oled_write_string(2, 7, "Restarted");
			restarted = 1;
		}		

		oled_write_string(0, 10, "BT Inactive");
		
		if ((result != OPDI_STATUS_OK) && (result != OPDI_CANCELLED)) {
			oled_write_string(0, 11, "Error:");
			fstr_dec(result, 3, 0, '.', dest);
			oled_write_string(8, 11, dest);				
		}

		info_beep();
		
		// inner loop to avoid unnecessary repaints (flickering)
		while (1) {
			
			uint32_t sleepTimer = timer0_ticks();
			
			while (!button_down(BUTTON_LEFT)
				&& (PIND & KEY_D_MIDDLE) 
				&& !button_down(BUTTON_RIGHT))
			{
				uint32_t ticks = timer0_ticks();
				
				if (ticks - sleepTimer > SLEEP_TICKS) {
					sleep();
					// wait until wakeup button released
					while (!(PIND & KEY_D_MIDDLE)) {}
					// reset sleep timer
					sleepTimer = timer0_ticks();
				}
			}
			
			// wait for button release
			button_wait_release_all();
			
			// check button
			if (!(PIND & KEY_D_MIDDLE)) {
				// wait until button released
				while (!(PIND & KEY_D_MIDDLE)) {}

				// start processing
				result = start();

				break;	// repaint				
			} else if (button_clicked(BUTTON_LEFT)) {
				sleep();
				// no repaint after wakeup
				
			}
		
		}	// while(1) to avoid repaint
	}	// main loop
	return 0;
}

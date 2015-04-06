//--------------------------------------------------------------------------------------
// 
// Code License: GNU GPL
//--------------------------------------------------------------------------------------
// Leo Meyer, leomeyer@gmx.de, 2015

#define F_CPU 	F_OSC
 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

#include "uart.h"
#include "timer0/timer0.h"

//-----------------------------------------------------------------------------

/*
 bitmask definitions of port code
 o p l r a b c d (lsb)
 ^
 |	^
 | | ^
 | | | ^
 | | | | ^ ^ ^ ^
 | | | | + + + + --- number of (virtual) port
 | | | + ----------- reserved 
 | | + ------------- line state (for output ports)
 | + --------------- pullup flag (for input ports only)
 + ----------------- output bit (msb; if set, port is output)
*/

// Output and pullup may not be used together.
// A value of 255 (all bits set) is the test code that returns the SIGNALCODE code.
// Any error case returns the SIGNALCODE code.

#define OUTPUT	7
#define PULLUP	6
#define LINESTATE 5
#define RESERVED 4
#define PORTMASK 0x0f

#define SIGNALCODE 	0xff

struct bits {
  uint8_t b0:1;
  uint8_t b1:1;
  uint8_t b2:1;
  uint8_t b3:1;
  uint8_t b4:1;
  uint8_t b5:1;
  uint8_t b6:1;
  uint8_t b7:1;
} __attribute__((__packed__)); 
#define BIT(name,pin,reg) \
 ((*(volatile struct bits*)&reg##name).b##pin) 

// virtual port definitions
#define VP0(reg) BIT(D,2,reg)
#define VP1(reg) BIT(D,3,reg)
#define VP2(reg) BIT(D,4,reg)
#define VP3(reg) BIT(D,5,reg)
#define VP4(reg) BIT(D,6,reg)
#define VP5(reg) BIT(D,7,reg)
#define VP6(reg) BIT(B,0,reg)
#define VP7(reg) BIT(B,1,reg)
#define VP8(reg) BIT(B,2,reg)
#define VP9(reg) BIT(B,3,reg)
#define VP10(reg) BIT(B,4,reg)
#define VP11(reg) BIT(B,5,reg)
#define VP12(reg) BIT(C,0,reg)
#define VP13(reg) BIT(C,1,reg)
#define VP14(reg) BIT(C,2,reg)
#define VP15(reg) BIT(C,3,reg)


/*************************************************************************
// main program
*************************************************************************/
int main(void)
{
	uint8_t data;
	uint8_t output;
	uint8_t pullup;
	uint8_t linestate;
	uint8_t portnumber;
	uint8_t in;
	
	CLKPR = (1<<7);   // enable prescaler register
	CLKPR = 0;        // prescale by 1	
	
	timer0_init();
	
	// setup uart
	initUART();
	
	// enable interrupts
	sei();

	while (1)                   // main loop
	{	
		data = getByte();

		// test byte?
		if (data == 0xff) {
			putByte(SIGNALCODE);

			continue;
		}
		
		output = (data >> OUTPUT) & 1;
		pullup = (data >> PULLUP) & 1;
		linestate = (data >> LINESTATE) & 1;
		portnumber = (data & PORTMASK);
		
		if (output + pullup == 2) {
			// error
			putByte(SIGNALCODE);
			continue;
		}
		if (output == 1) {
				VP0(DDR)  = 1; VP0(PORT)  = 1;
				_delay_ms(500);       // halbe sekunde warten
				VP0(DDR)  = 1; VP0(PORT)  = 0;
				_delay_ms(500);       // halbe sekunde warten
		
			switch (portnumber) {
				case 0:  VP0(DDR)  = 1; VP0(PORT)  = linestate;  break;
				case 1:  VP1(DDR)  = 1; VP1(PORT)  = linestate;  break;
				case 2:  VP2(DDR)  = 1; VP2(PORT)  = linestate;  break;
				case 3:  VP3(DDR)  = 1; VP3(PORT)  = linestate;  break;
				case 4:  VP4(DDR)  = 1; VP4(PORT)  = linestate;  break;
				case 5:  VP5(DDR)  = 1; VP5(PORT)  = linestate;  break;
				case 6:  VP6(DDR)  = 1; VP6(PORT)  = linestate;  break;
				case 7:  VP7(DDR)  = 1; VP7(PORT)  = linestate;  break;
				case 8:  VP8(DDR)  = 1; VP8(PORT)  = linestate;  break;
				case 9:  VP9(DDR)  = 1; VP9(PORT)  = linestate;  break;
				case 10: VP10(DDR) = 1; VP10(PORT) = linestate; break;
				case 11: VP11(DDR) = 1; VP11(PORT) = linestate; break;
				case 12: VP12(DDR) = 1; VP12(PORT) = linestate; break;
				case 13: VP13(DDR) = 1; VP13(PORT) = linestate; break;
				case 14: VP14(DDR) = 1; VP14(PORT) = linestate; break;
				case 15: VP15(DDR) = 1; VP15(PORT) = linestate; break;
			}
			
			// return the same byte
			putByte(data);
		} else {
		
			// input
			switch (portnumber) {
				case 0:  VP0(DDR)  = 0; VP0(PORT)  = pullup;  break;
				case 1:  VP1(DDR)  = 0; VP1(PORT)  = pullup;  break;
				case 2:  VP2(DDR)  = 0; VP2(PORT)  = pullup;  break;
				case 3:  VP3(DDR)  = 0; VP3(PORT)  = pullup;  break;
				case 4:  VP4(DDR)  = 0; VP4(PORT)  = pullup;  break;
				case 5:  VP5(DDR)  = 0; VP5(PORT)  = pullup;  break;
				case 6:  VP6(DDR)  = 0; VP6(PORT)  = pullup;  break;
				case 7:  VP7(DDR)  = 0; VP7(PORT)  = pullup;  break;
				case 8:  VP8(DDR)  = 0; VP8(PORT)  = pullup;  break;
				case 9:  VP9(DDR)  = 0; VP9(PORT)  = pullup;  break;
				case 10: VP10(DDR) = 0; VP10(PORT) = pullup; break;
				case 11: VP11(DDR) = 0; VP11(PORT) = pullup; break;
				case 12: VP12(DDR) = 0; VP12(PORT) = pullup; break;
				case 13: VP13(DDR) = 0; VP13(PORT) = pullup; break;
				case 14: VP14(DDR) = 0; VP14(PORT) = pullup; break;
				case 15: VP15(DDR) = 0; VP15(PORT) = pullup; break;
			}
			// wait for the input to stabilize
			_delay_ms(1);
			switch (portnumber) {
				case 0:  in = VP0(PIN);  break;
				case 1:  in = VP1(PIN);  break;
				case 2:  in = VP2(PIN);  break;
				case 3:  in = VP3(PIN);  break;
				case 4:  in = VP4(PIN);  break;
				case 5:  in = VP5(PIN);  break;
				case 6:  in = VP6(PIN);  break;
				case 7:  in = VP7(PIN);  break;
				case 8:  in = VP8(PIN);  break;
				case 9:  in = VP9(PIN);  break;
				case 10: in = VP10(PIN); break;
				case 11: in = VP11(PIN); break;
				case 12: in = VP12(PIN); break;
				case 13: in = VP13(PIN); break;
				case 14: in = VP14(PIN); break;
				case 15: in = VP15(PIN); break;
				default: in = 0;
			}
			// set line state in returned byte
			data &= ~(1 << LINESTATE);
			if (in == 1)
				data |= (1 << LINESTATE);
			putByte(data);
		}
		
	}	// main loop
	return 0;
}

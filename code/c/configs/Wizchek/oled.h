#ifndef __OLED_H
#define __OLED_H
//--------------------------------------------------------------------------------------
// ATmega168PV 8MHz - OLED-Demo SSD1325  (im Geraet lauffaehig)
// Akku-Chek Compact Plus
// Author: adriaNo6
//--------------------------------------------------------------------------------------
 
#define F_CPU 	F_OSC
 
#include <avr/io.h>
#include <avr/sleep.h>
#include <compat/deprecated.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define PORTRAIT 1
#define LANDSCAPE 2
#define OLED_FORMAT PORTRAIT		// Hochformat
//#define OLED_FORMAT LANDSCAPE		// Querformat

#ifndef OLED_FORMAT
	#error "OLED_FORMAT not defined"
#endif

#if (OLED_FORMAT == LANDSCAPE)
	#define OLED_WIDTH 		17
	#define OLED_HEIGHT 	10
#elif (OLED_FORMAT == PORTRAIT)
	#define OLED_WIDTH 		13
	#define OLED_HEIGHT 	12
#endif

#ifndef OLED_WIDTH
	#error "OLED_FORMAT specifier invalid"
#endif

#define OLED_B_PIN_CS		(1 << PB1)
#define OLED_B_PIN_LOAD		(1 << PB0)

#define OLED_C_PIN_RST		(1 << PC2)
#define OLED_C_PIN_DC		(1 << PC1)

#define OLED_D_PIN_12V		(1 << PD5)
#define OLED_D_PIN_SCLK		(1 << PD4)
#define OLED_D_PIN_SDIN		(1 << PD1)

#define BYTE unsigned char

void write_byte(unsigned char dat_or_cmd);
void write_cmd(unsigned char command);
void write_dat(unsigned char data);

void delay(unsigned char);
void oled_init(void);
void oled_on(void);
void oled_off(void);
void oled_clrscr(void);
// zwei Varianten (hoch/quer)
void PaintChar(BYTE sign, BYTE column, BYTE row);
void PaintPic(BYTE *Ptr, BYTE column, BYTE row);

// Defines the OLED color or grayscale value.
// If the highest bit is set, indicates that text should be inverted.
void oled_color(BYTE color);

void oled_write_string(BYTE xPos, BYTE yPos, const char *Str);
void oled_write_char(BYTE xPos, BYTE yPos, const char c);
#endif

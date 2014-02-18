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
#include <avr/interrupt.h>

#include "oled.h"
#include "timer0/timer0.h"

//-----------------------------------------------------------------------------


#define OLED_12V_HI sbi( PORTD, PORTD5) //PORTD.5
#define OLED_12V_LO cbi( PORTD, PORTD5) //PORTD.5

#define OLED_SDIN_HI  sbi( PORTD, PORTD1) //PORTD:xxxx.xxIx
#define OLED_SDIN_LO  cbi( PORTD, PORTD1);

#define OLED_SCLK_HI  sbi( PORTD, PORTD4); //PORTD:xxxx.xxIx
#define OLED_SCLK_LO  cbi( PORTD, PORTD4);

#define OLED_DC_HI  sbi( PORTC, PORTC1); //PORTD:xxxx.xxIx
#define OLED_DC_LO  cbi( PORTC, PORTC1);

#define OLED_RES_HI  sbi( PORTC, PORTC2); //PORTD:xxxx.xxIx
#define OLED_RES_LO  cbi( PORTC, PORTC2);

#define OLED_CS_HI  sbi( PORTB, PORTB1); //PORTD:xxxx.xxIx
#define OLED_CS_LO  cbi( PORTB, PORTB1);

#define OLED_LOAD_HI sbi( PORTB, PORTB0);   // connect 100 Ohm to GND
#define OLED_LOAD_LO cbi( PORTB, PORTB0);   // disconnect 100 Ohm to GND


static const unsigned char PROGMEM code5x7[] = {           //97x5 Byte
  0x00, 0x00, 0x00, 0x00, 0x00,  // sp           0x20
  0x00, 0x00, 0x5f, 0x00, 0x00,  // !            0x21
  0x00, 0x07, 0x00, 0x07, 0x00,  // "            0x22
  0x14, 0x7f, 0x14, 0x7f, 0x14,  // #            0x23
  0x24, 0x2a, 0x7f, 0x2a, 0x12,  // $            0x24
  0xc4, 0xc8, 0x10, 0x26, 0x46,  // %            0x25
  0x36, 0x49, 0x55, 0x22, 0x50,  // &            0x26
  0x02, 0x05, 0x02, 0x00, 0x00,  // Grad         0x27
  0x00, 0x1c, 0x22, 0x41, 0x00,  // (            0x28
  0x00, 0x41, 0x22, 0x1c, 0x00,  // )            0x29
  0x14, 0x08, 0x3E, 0x08, 0x14,  // *            0x2A
  0x08, 0x08, 0x3E, 0x08, 0x08,  // +            0x2B
  0x00, 0x00, 0x50, 0x30, 0x00,  // ,            0x2C
  0x10, 0x10, 0x10, 0x10, 0x10,  // -            0x2D
  0x00, 0x60, 0x60, 0x00, 0x00,  // .            0x2E
  0x20, 0x10, 0x08, 0x04, 0x02,  // /            0x2F
  0x3E, 0x51, 0x49, 0x45, 0x3E,  // 0            0x30
  0x00, 0x42, 0x7F, 0x40, 0x00,  // 1            0x31
  0x42, 0x61, 0x51, 0x49, 0x46,  // 2            0x32
  0x22, 0x41, 0x49, 0x49, 0x36,  // 3            0x33
  0x18, 0x14, 0x12, 0x7F, 0x10,  // 4            0x34
  0x27, 0x45, 0x45, 0x45, 0x39,  // 5            0x35
  0x3C, 0x4A, 0x49, 0x49, 0x30,  // 6            0x36
  0x01, 0x71, 0x09, 0x05, 0x03,  // 7            0x37
  0x36, 0x49, 0x49, 0x49, 0x36,  // 8            0x38
  0x06, 0x49, 0x49, 0x29, 0x1E,  // 9            0x39
  0x00, 0x36, 0x36, 0x00, 0x00,  // :            0x3A
  0x00, 0x56, 0x36, 0x00, 0x00,  // ;            0x3B
  0x08, 0x14, 0x22, 0x41, 0x00,  // <            0x3C
  0x14, 0x14, 0x14, 0x14, 0x14,  // =            0x3D
  0x00, 0x41, 0x22, 0x14, 0x08,  // >            0x3E
  0x02, 0x01, 0x51, 0x09, 0x06,  // ?            0x3F
  0x32, 0x49, 0x59, 0x51, 0x3E,  // @            0x40
  0x7E, 0x11, 0x11, 0x11, 0x7E,  // A            0x41
  0x7F, 0x49, 0x49, 0x49, 0x36,  // B            0x42
  0x3E, 0x41, 0x41, 0x41, 0x22,  // C            0x43
  0x7F, 0x41, 0x41, 0x22, 0x1C,  // D            0x44
  0x7F, 0x49, 0x49, 0x49, 0x41,  // E            0x45
  0x7F, 0x09, 0x09, 0x09, 0x01,  // F            0x46
  0x3E, 0x41, 0x49, 0x49, 0x7A,  // G            0x47
  0x7F, 0x08, 0x08, 0x08, 0x7F,  // H            0x48
  0x00, 0x41, 0x7F, 0x41, 0x00,  // I            0x49
  0x20, 0x40, 0x41, 0x3F, 0x01,  // J            0x4A
  0x7F, 0x08, 0x14, 0x22, 0x41,  // K            0x4B
  0x7F, 0x40, 0x40, 0x40, 0x40,  // L            0x4C
  0x7F, 0x02, 0x0C, 0x02, 0x7F,  // M            0x4D
  0x7F, 0x04, 0x08, 0x10, 0x7F,  // N            0x4E
  0x3E, 0x41, 0x41, 0x41, 0x3E,  // O            0x4F
  0x7F, 0x09, 0x09, 0x09, 0x06,  // P            0x50
  0x3E, 0x41, 0x51, 0x21, 0x5E,  // Q            0x51
  0x7F, 0x09, 0x19, 0x29, 0x46,  // R            0x52
  0x46, 0x49, 0x49, 0x49, 0x31,  // S            0x53
  0x01, 0x01, 0x7F, 0x01, 0x01,  // T            0x54
  0x3F, 0x40, 0x40, 0x40, 0x3F,  // U            0x55
  0x1F, 0x20, 0x40, 0x20, 0x1F,  // V            0x56
  0x3F, 0x40, 0x38, 0x40, 0x3F,  // W            0x57
  0x63, 0x14, 0x08, 0x14, 0x63,  // X            0x58
  0x07, 0x08, 0x70, 0x08, 0x07,  // Y            0x59
  0x61, 0x51, 0x49, 0x45, 0x43,  // Z            0x5A
  0x00, 0x7F, 0x41, 0x41, 0x00,  // [ -          0x5B
  0x55, 0x2A, 0x55, 0x2A, 0x55,  // backslash    0x5C
  0x00, 0x41, 0x41, 0x7F, 0x00,  // ]            0x5D
  0x04, 0x02, 0x01, 0x02, 0x04,  // ^            0x5E
  0x40, 0x40, 0x40, 0x40, 0x40,  // _            0x5F
  0x00, 0x01, 0x02, 0x04, 0x00,  // '            0x60
  0x20, 0x54, 0x54, 0x54, 0x78,  // a            0x61
  0x7F, 0x48, 0x44, 0x44, 0x38,  // b            0x62
  0x38, 0x44, 0x44, 0x44, 0x20,  // c            0x63
  0x38, 0x44, 0x44, 0x48, 0x7F,  // d            0x64
  0x38, 0x54, 0x54, 0x54, 0x18,  // e            0x65
  0x08, 0x7E, 0x09, 0x01, 0x02,  // f            0x66
  0x0C, 0x52, 0x52, 0x52, 0x3E,  // g            0x67
  0x7F, 0x08, 0x04, 0x04, 0x78,  // h            0x68
  0x00, 0x44, 0x7D, 0x40, 0x00,  // i            0x69
  0x20, 0x40, 0x44, 0x3D, 0x00,  // j            0x6A
  0x7F, 0x10, 0x28, 0x44, 0x00,  // k            0x6B
  0x00, 0x41, 0x7F, 0x40, 0x00,  // l            0x6C
  0x7C, 0x04, 0x18, 0x04, 0x78,  // m            0x6D
  0x7C, 0x08, 0x04, 0x04, 0x78,  // n            0x6E
  0x38, 0x44, 0x44, 0x44, 0x38,  // o            0x6F
  0x7C, 0x14, 0x14, 0x14, 0x08,  // p            0x70
  0x08, 0x14, 0x14, 0x18, 0x7C,  // q            0x71
  0x7C, 0x08, 0x04, 0x04, 0x08,  // r            0x72
  0x48, 0x54, 0x54, 0x54, 0x20,  // s            0x73
  0x04, 0x3F, 0x44, 0x40, 0x20,  // t            0x74
  0x3C, 0x40, 0x40, 0x20, 0x7C,  // u            0x75
  0x1C, 0x20, 0x40, 0x20, 0x1C,  // v            0x76
  0x3C, 0x40, 0x30, 0x40, 0x3C,  // w            0x77
  0x44, 0x28, 0x10, 0x28, 0x44,  // x            0x78
  0x0C, 0x50, 0x50, 0x50, 0x3C,  // y            0x79
  0x44, 0x64, 0x54, 0x4C, 0x44,  // z            0x7A                                             
  0x00, 0x08, 0x36, 0x41, 0x00,  // {            0x7B
  0x00, 0x00, 0x7F, 0x00, 0x00,  // |            0x7C
  0x00, 0x41, 0x36, 0x08, 0x00,  // }            0x7D
  0x08, 0x08, 0x2A, 0x1C, 0x08,  // ->           0x7E
  0x08, 0x1C, 0x2A, 0x08, 0x08   // <-           0x7F
};


static BYTE color_value = 0xF;		// whitest, not inverted
static uint8_t DisplayOnOff;

void doCopy(uint8_t scol, uint8_t srow, uint8_t ecol, uint8_t erow, uint8_t ncol, uint8_t nrow) {
	write_cmd(0x23); // enable x-wraparound
	write_cmd(2);
	write_cmd(0x25); // enter copy mode
	write_cmd(scol); // start column
	write_cmd(srow); // start row
	write_cmd(ecol); // end column
	write_cmd(erow); // end row
	write_cmd(ncol); // new column
	write_cmd(nrow); // new row
}



void scroll_on(void) {
	write_cmd(0x23);		// Disable wrap during scrolling
	write_cmd(0x00);
	write_cmd(0x26);		// Horz. Scrolling
	write_cmd(0x01);		// by 1 col.
	write_cmd(80);			// # of rows
	write_cmd(0x00);		// time interval 12 frames
	write_cmd(0x2f);		// Activate scrolling
}


void scroll_off(void) {
	write_cmd(0x2e);		// Deactivate scrolling
}


//----------------------------------------------------------------------------------------
// OLED-Display initialisieren SSD1325
//----------------------------------------------------------------------------------------
void oled_init(void) 
{
  write_cmd(0x15);           // Set Column Address
  write_cmd(0x07);           // Begin  7 Offset!
  write_cmd(0x39);           // End   57 

  write_cmd(0x75);           // Set Row Address
  write_cmd(0x00);           // Begin  0
  write_cmd(0x4F);           // End   79

  write_cmd(0x86);           // Set Current Range 84h:Quarter, 85h:Half, 86h:Full
  write_cmd(0x81);           // Set Contrast Control
  write_cmd(0x2D);
  write_cmd(0xBE);           // Set VCOMH Voltage
  write_cmd(0x11);           // write_cmd(0x11);  --> bei Streifenfehlern ?  (00 default)

  write_cmd(0xBC);           // Set Precharge Voltage
  write_cmd(0x0F);

  write_cmd(0xA0);           // Set Re-map
  write_cmd(0x41);           //od. 0x43

  write_cmd(0xA6);           // Entire Display OFF, all pixels turns OFF
  write_cmd(0xA8);           // Set Multiplex Ratio
  write_cmd(0x4F);           // multiplex ratio N from 16MUX-80MUX // 80!
  write_cmd(0xB1);           // Set Phase Length
  write_cmd(0x22);
  write_cmd(0xB2);           // Set Row Period
  write_cmd(0x46);
  write_cmd(0xB0);           // Set Pre-charge Compensation Enable
  write_cmd(0x08);
  write_cmd(0xB4);           // Set Pre-charge Compensation Level
  write_cmd(0x00);
  write_cmd(0xB3);           // Set Display Clock
  write_cmd(0xA0);
  write_cmd(0xBF);           // Set Segment Low Voltage (VSL)
  write_cmd(0x0D);

  write_cmd(0xB8);           // Set Gray Scale Table
  write_cmd(0x01);
  write_cmd(0x11);
  write_cmd(0x22);
  write_cmd(0x32);
  write_cmd(0x43);
  write_cmd(0x54);
  write_cmd(0x65);
  write_cmd(0x76);
                             
  write_cmd(0xAE);           // Set Display ON
  write_cmd(0xAD);           // Set Master Configuration
  write_cmd(0x02);
  timer0_delay(75);              // dannach 75ms Pause!? 7365us
  write_cmd(0xA4);           // Normal Display
  write_cmd(0xAF);           // Set Display OFF
  write_cmd(0xE3);           //NOP
}


void oled_on(void) {
	OLED_RES_LO;               // Reset
	_delay_us(10);
	OLED_RES_HI;

	oled_init();

	OLED_LOAD_HI;              // 100 Ohm to GND
	OLED_12V_HI;               // Vcc 12V on
	_delay_ms(40);             // wait until VCC stable
	OLED_LOAD_LO;              // disconnect 100 Ohm to GND
	write_cmd(0xAF);           // Set Display ON
	DisplayOnOff = 1;
}

void oled_off(void) {
	write_cmd(0xAE);           // Set Display OFF
	OLED_12V_LO;               // Vcc 12V off
	OLED_LOAD_HI;              // 100 Ohm to GND
	_delay_ms(200);            // Wait until panel discharges completely
	OLED_LOAD_LO;              // disconnect 100 Ohm to GND
	DisplayOnOff = 0;
}

//----------------------------------------------------------------------------------------
// Display loeschen
//----------------------------------------------------------------------------------------
void oled_clrscr(void)
{
  unsigned int a;

  write_cmd(0x15);
  write_cmd(0x07);
  write_cmd(0x39);
  write_cmd(0x75);
  write_cmd(0x00);
  write_cmd(0x4F);
  for (a=0;a<(102/2*80);a++) write_dat(0x00);        //clr
}


// ------------------------------------------------------------------
//  Bitmaps ans Oled-Display senden
// ------------------------------------------------------------------
void PaintPic(BYTE *Ptr, unsigned char column, unsigned char row) 
{
  unsigned char CharCol_1=0, CharCol_2=0;
  unsigned char char_r_y_p12, char_r_y_p34, char_r_y_p56;
  unsigned char pz, y,x,z,tmp;
  unsigned int pos,bitmode,hoehe,breite;
  tmp = column;
  pos = 0;

  bitmode = pgm_read_byte(&Ptr[pos++]);
  hoehe = pgm_read_byte(&Ptr[pos++]);
  breite = pgm_read_byte(&Ptr[pos++]);
  if (bitmode==1) 
  {
    for (z=0; z<6; z++) 
    {
      for (x=0; x<42; x++) 
      {
        CharCol_1 = pgm_read_byte(&Ptr[pos++]);
        CharCol_2 = pgm_read_byte(&Ptr[pos++]);
        write_cmd(0x15);
        write_cmd(column+7);
        write_cmd(column+7);
        write_cmd(0x75);
        write_cmd(8*row);
        write_cmd(8*row+7);

        pz = 1;                                                  //pixelzeile
        for (y=0; y<8; y++) 
        {
          char_r_y_p12 = char_r_y_p34 = char_r_y_p56 = 0;
          if (CharCol_1 & pz) { char_r_y_p12 |= 0xF0; }
          if (CharCol_2 & pz) { char_r_y_p12 |= 0x0F; }
          write_dat(char_r_y_p12);
          pz = pz << 1;
        }
        column++;
      } // for < 42
      column=tmp;
      row++;
    } // z 10 Zeilen
  }
  else                       // bitmode: 4
  {
    pos = (hoehe * breite)/2-1+3;
    for (z=0; z<hoehe; z++)  // Zeilen
    {
      write_cmd(0x15);       // Spalte
      write_cmd(0+7);
      write_cmd((breite/2-1)+7);
      write_cmd(0x75);       // Reihe
      write_cmd(z);
      write_cmd(z);
      for (y=0; y<(breite/2); y++)
      {
        tmp = pgm_read_byte(&Ptr[pos--]);
        x = tmp>>4;
        tmp = tmp<<4;
        tmp = tmp+x;
        write_dat(tmp);     
      }
    }
  }
}

#ifndef OLED_FORMAT
	#error "OLED_FORMAT not defined"
#endif

#if (OLED_FORMAT == LANDSCAPE)
//------------------------------------------------------------
// Funktion stellt ein Zeichen dar 5x7 (Querformat)
// column 00..16 Spalten 17
// row     0...9 Zeilen  10
// ------------------------------------------------------------------
void PaintChar(unsigned char sign, unsigned char column, unsigned char row) 
{
  unsigned char CharCol_1, CharCol_2, CharCol_3, CharCol_4, CharCol_5;
  unsigned char char_r_y_p12, char_r_y_p34, char_r_y_p56;
  unsigned char pz, y;
  unsigned int pos;
  
  if ((sign<0x20) || (sign>0x7F)) { sign = 0x20; } 
  pos = 5*(sign-0x20);

  CharCol_1 = pgm_read_byte(&code5x7[pos++]);
  CharCol_2 = pgm_read_byte(&code5x7[pos++]);
  CharCol_3 = pgm_read_byte(&code5x7[pos++]);
  CharCol_4 = pgm_read_byte(&code5x7[pos++]);
  CharCol_5 = pgm_read_byte(&code5x7[pos++]);

  write_cmd(0x15);
  write_cmd(7+3*column);
  write_cmd(7+3*column+2);
  write_cmd(0x75);
  write_cmd(8*row);
  write_cmd(8*row+7);
  
  pz = 1;                                                  //pixelzeile
  for (y=0; y<8; y++) 
  {
    char_r_y_p12 = char_r_y_p34 = char_r_y_p56 = 0;
    if (CharCol_1 & pz) { char_r_y_p12 |= 0xF0; }
    if (CharCol_2 & pz) { char_r_y_p12 |= 0x0F; }
    write_dat(char_r_y_p12);
    if (CharCol_3 & pz) { char_r_y_p34 |= 0xF0; }
    if (CharCol_4 & pz) { char_r_y_p34 |= 0x0F; }
    write_dat(char_r_y_p34);
    if (CharCol_5 & pz) { char_r_y_p56 = 0xF0; }           // 6.Spalte bleibt immer leer
    write_dat(char_r_y_p56);
    pz = pz << 1;
  }
}

#elif (OLED_FORMAT == PORTRAIT)

//-------------------------------------------------------------------------------------------------
// Zeichendarstellung im Display-Hochformat
//
// Hoehe x Breite: 102x80 Pixel
//
//    row: 0..11 = 12 Zeilen
// column: 0..12 = 13 Spalten
//
//-------------------------------------------------------------------------------------------------
void PaintChar(BYTE sign, BYTE row, BYTE column) 
{
unsigned int pos;
BYTE tmp, twopix, y;

  if (row>12) { return; }
  if (column>11) { return; }

  write_cmd(0x15);
  write_cmd(10+4*(11-column));
  write_cmd(10+4*(11-column)+3);   // 8 Spalten a 2 Pixel
  write_cmd(0x75);
  write_cmd(6*row);
  write_cmd(6*row+5);              // 6 Zeilen

  if ((sign<0x20) || (sign>0x7F)) { sign = 0x20; } 
  pos = 5*(sign-0x20);

  for (y=0; y<6; y++)
  {
    twopix = 0;
    tmp = pgm_read_byte(&code5x7[pos++]);
    if (y>4) { tmp = 0; }                        // leere Pixelspalte: sonst steht Zeichen an Zeichen!
	// inverted?
	if ((color_value & 128) == 128) tmp = ~tmp;
    if (tmp & 0x80) { twopix |= (color_value & 0x0F) << 4; }
    if (tmp & 0x40) { twopix |= (color_value & 0x0F); }
    write_dat(twopix); twopix = 0;
    if (tmp & 0x20) { twopix |= (color_value & 0x0F) << 4; }
    if (tmp & 0x10) { twopix |= (color_value & 0x0F); }
    write_dat(twopix); twopix = 0;
    if (tmp & 0x08) { twopix |= (color_value & 0x0F) << 4; }
    if (tmp & 0x04) { twopix |= (color_value & 0x0F); }
    write_dat(twopix); twopix = 0;
    if (tmp & 0x02) { twopix |= (color_value & 0x0F) << 4; }
    if (tmp & 0x01) { twopix |= (color_value & 0x0F); }
    write_dat(twopix);
  }
}

#else
	#error "OLED_FORMAT must be either PORTRAIT or LANDSCAPE"
#endif


//-----------------------------------------------------------------------------
// send a data byte to oled controller
//-----------------------------------------------------------------------------
void write_dat(unsigned char data)
{
  OLED_CS_LO;
  OLED_DC_HI;
  write_byte(data);
  OLED_CS_HI;
}


//-----------------------------------------------------------------------------
// send a command byte to oled controller
//-----------------------------------------------------------------------------
void write_cmd(unsigned char command) 
{
  OLED_CS_LO;
  OLED_DC_LO;
  write_byte(command);
  OLED_DC_HI;
}


//-----------------------------------------------------------------------------
// send a data or command byte
//-----------------------------------------------------------------------------
void write_byte(unsigned char dat_or_cmd)
{
  int i;

  for(i=0; i<8; i++)
  {
    if (dat_or_cmd & 0x80) { OLED_SDIN_HI; } 
    else                   { OLED_SDIN_LO; }
    dat_or_cmd = dat_or_cmd << 1;
    OLED_SCLK_LO;    
    OLED_SCLK_HI;
  }
  OLED_SDIN_LO;
}

void oled_color(BYTE color) {
	color_value = color;
}

//-----------------------------------------------------------------------------
// send a string to oled
// Orientation depends on OLED_FORMAT setting
// xPos: 0..16 (17 Spalten a 6 Pixel breit)
// yPos: 0.. 9 (10 Zeilen a 8 Pixel breit)
// Str:  max 17 Zeiche a 
//-----------------------------------------------------------------------------
void oled_write_string(BYTE xPos, BYTE yPos, const char *Str) 
{
  BYTE c,i;

  i = xPos;
  while ((*Str!=0) && (i<17)) 
  { 
    c = *Str;
    PaintChar(c,i++,yPos);
    Str++;
  }
}


//-----------------------------------------------------------------------------
// send a string to oled
// Orientation depends on OLED_FORMAT setting
// xPos: 0..16 (17 Spalten a 6 Pixel breit)
// yPos: 0.. 9 (10 Zeilen a 8 Pixel breit)
//-----------------------------------------------------------------------------
void oled_write_char(BYTE xPos, BYTE yPos, const char c) 
{
  PaintChar(c, xPos, yPos);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void delay(unsigned char d)
{
  unsigned char i, j;
  for (i=0; i<d; i++)
	for (j=0; j<255; j++);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

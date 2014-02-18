#ifndef _BEEP_H___
#define _BEEP_H___

/************************************************************************/
/*                      AVRTools                                        */
/*                      Beeper routines									*/
/*                      Version 0.1                                     */
/* Code License: unknown (assume GNU GPL)								*/
/************************************************************************/
// Leo Meyer, 12/2009

#include <avr/io.h>
#include <stdint.h>

#define OUTPUT_PIN	PB2

#define TONE_C		261
#define TONE_Csharp	277	
#define TONE_Dflat	277	
#define TONE_D		294
#define TONE_Dsharp	311
#define TONE_Eflat	311
#define TONE_E		330
#define TONE_Fflat	330
#define TONE_Esharp	349
#define TONE_F		349
#define TONE_Fsharp	370
#define TONE_Gflat	370
#define TONE_G		392
#define TONE_Gsharp	415
#define TONE_Aflat	415
#define TONE_A		440
#define TONE_Asharp	466
#define TONE_Bflat	466
#define TONE_B		494


extern void sound_on(uint8_t ocr);

extern void sound_off(void);

// Beep at the given frequency with the specified duration in clock ticks
extern void tone(uint16_t freq, uint32_t duration);


// Beep at the given scale with the specified duration in clock ticks
extern void beep(uint16_t scale, uint32_t duration);

#endif

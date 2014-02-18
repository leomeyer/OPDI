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

#ifndef _FORMAT_H___
#define _FORMAT_H___


/************************************************************************/
/*                      AVRTools                                        */
/*                      Formatting routines								*/
/*																		*/
/*                      Version 0.1                                     */
/************************************************************************/
// Leo Meyer, 12/2009

#ifndef WIN32
#include <stdint.h>
#else

// for debugging in Visual Studio

#define uint8_t	int
#define int8_t	int
#define int32_t	int
#define bool int
#define false 0
#define true 1
#endif

#define FORMAT_STANDARD	0
#define FORMAT_FILLZEROES	1

// Create a nicely formatted decimal number within a char*
// val: value (up to 32 bits signed)
// prec: digits before separator
// dec: digits after separator
// sep: separator character
// dest: target pointer
// Formats using right alignment and fills up to the left with blanks.
// The value is treated as scaled by 10 to power of dec.
// dest must be big enough to hold (prec + 2) characters if dec is 0,
// because of the sign character and the trailing \0;
// if dec is greater than 0, dest must hold (prec + dec + 3) characters,
// because the separator is inserted, too. If the value is too large to
// be represented within this space, the result contains an asterisk (*)
// as the first character. If the value is negative, a minus sign is
// inserted before the first digit; in this case the minimum value
// is reduced by 1/10. On overflow the asterisk will be put into the
// second position.
// The function returns 0 if everything is ok.
// When an overflow occurs the result is 1.
// On invalid input (prec < 0 etc.) the result is 2. In this case
// the content of dest is undefined.
// Examples:
//   fstr_dec(0, 2, 0, '.', dest) ==> "  0" 
//   fstr_dec(0, 2, 1, '.', dest) ==> "  0.0" 
//   fstr_dec(0, 2, 2, '.', dest) ==> "  0.00" 
//   fstr_dec(12, 2, 0, '.', dest) ==> " 12" 
//   fstr_dec(12, 2, 1, '.', dest) ==> "  1.2" 
//   fstr_dec(12, 2, 2, '.', dest) ==> "  0.12" 
//   fstr_dec(-12, 2, 0, '.', dest) ==> "-12" 
//   fstr_dec(-12, 2, 1, '.', dest) ==> " -1.2" 
//   fstr_dec(-12, 2, 2, '.', dest) ==> " -0.12" 
extern uint8_t fstr_dec_f(const int32_t val, uint8_t prec, uint8_t dec, char sep, char *dest, uint8_t flags);

// convenience function for the default case without leading zeroes
extern uint8_t fstr_dec(const int32_t val, uint8_t prec, uint8_t dec, char sep, char *dest);

// Converts a decimal representation into its 32 bit value.
// Only allowed characters are '-', sep (both only once), and 0-9.
// In case of an error result contains a value < 0. The return value is undefined.
// Otherwise, result contains the number of decimals of the parsed value.
extern int32_t fstr2int(char* buffer, char sep, int8_t *result);

#endif

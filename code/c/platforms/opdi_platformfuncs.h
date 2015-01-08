//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-2014 Leo Meyer (leo@leomeyer.de)
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
    
// Platform specific functions
//
// A config implementation must provide these functions which are usually
// platform specific.

#ifndef __OPDI_PLATFORMFUNCS_H
#define __OPDI_PLATFORMFUNCS_H

#include "opdi_platformtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Determines whether a character is a space on the platform.
* 	Returns 0 if it isn't.
*/
extern uint8_t opdi_is_space(char c);

/** Tries to parse the string in str to a number with base 10. 
    If the value can be parsed, stores the result and returns 0.
    Otherwise, returns a value != 0.
*/
extern uint8_t opdi_str_to_uint8(const char *str, uint8_t *result);

/** Tries to parse the string in str to a number with base 10. 
    If the value can be parsed, stores the result and returns 0.
    Otherwise, returns a value != 0.
*/
extern uint8_t opdi_str_to_uint16(const char *str, uint16_t *result);

/** Tries to parse the string in str to a number with base 10. 
    If the value can be parsed, stores the result and returns 0.
    Otherwise, returns a value != 0.
*/
extern uint8_t opdi_str_to_int32(const char *str, int32_t *result);

/** Formats the value into a string (base 10). Returns the length of the string.
* The buffer must be large enough to hold the largest possible value (4 characters).
*/
extern uint8_t opdi_uint8_to_str(uint8_t value, char* msgBuf);

/** Formats the value into a string (base 10). Returns the length of the string.
* The buffer must be large enough to hold the largest possible value (6 characters).
*/
extern uint8_t opdi_uint16_to_str(uint16_t value, char* msgBuf);

/** Formats the value into a string (base 10). Returns the length of the string.
* The buffer must be large enough to hold the largest possible value (10 characters).
*/
extern uint8_t opdi_int32_to_str(int32_t value, char* msgBuf);

/** Decodes a sequence of bytes into a char array.
*   May indicate an error by returning a value != 0.
*/
extern uint8_t opdi_bytes_to_string(uint8_t bytes[], uint16_t offset, uint16_t length, char *dest, uint16_t dest_length);

/** Encodes a sequence of chars into a byte array.
*   May indicate an error by returning a value != 0. Puts the byte count into bytelen.
*/
extern uint8_t opdi_string_to_bytes(char* string, uint8_t *dest, uint16_t pos, uint16_t maxlength, uint16_t *bytelen);

/** Case insensitive comparison. Platforms that do not support this may implement a case sensitive comparison.
*   Semantics like strcmp.
*/
extern uint8_t opdi_string_cmp(const char *s1, const char *s2);

/** Returns current system time in milliseconds. 
*/
extern uint64_t opdi_get_time_ms(void);

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_PLATFORMFUNCS_H


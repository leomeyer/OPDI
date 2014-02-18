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
    
// Common string functions

#ifndef __OPDI_STRINGS_H
#define __OPDI_STRINGS_H

#include "opdi_platformtypes.h"

#ifdef __cplusplus
extern "C" {
#endif 

/** Splits a null-terminated string at the given separator into substrings.
*   Substrings may contain the escaped separator (twice in a row) which is substituted
*   in-place. Returns the number of parts found in part_count if it is != NULL. 
*   This routine also trims the parts of whitespace if trim is != 0. 
*   In case of an error it returns a value != 0.
*/
extern uint8_t strings_split(const char *str, char separator, const char **parts, uint8_t max_parts, uint8_t trim, uint8_t *part_count);

/** Joins the null-terminated strings in the parts array by the separator until one entry in parts is NULL.
*   The output is written to dest. If the separator is contained in any of the strings it is escaped.
*   Empty parts are encoded as a single blank.
*   If max_length is exceeded a value != 0 is returned.
*/
extern uint8_t strings_join(const char **parts, char separator, char *dest, uint16_t max_length);

#ifdef __cplusplus
}
#endif

#endif		// __OPDI_STRINGS_H
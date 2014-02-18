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

#include <stdlib.h>

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"
#include "opdi_strings.h"

// TODO this has to be redone to preserve whitespace correctly

uint8_t strings_split(const char *str, char separator, const char **parts, uint8_t max_parts, uint8_t trim, uint8_t *part_count) {
	char *dPtr = (char *)str;
	uint16_t dPos = 0;	// destination position
	uint16_t sPos = 0;	// source position
	uint8_t partCount = 1;
	uint8_t i;

	if (max_parts == 0) return 0;

	// initial part
	parts[0] = dPtr;
	// loop through string
	while (str[sPos]) {
		if (str[sPos] == separator) {
			// lookahead; this is safe because we expect at least a terminating \0
			if (str[sPos + 1] == separator) {
				// escaped separator found
				dPtr[dPos++] = separator;
				sPos++;
			} else {
				// separator found; part is finished
				dPtr[dPos] = '\0';
				// set the next pointer to the new part
				dPtr = (char *)str + sPos + 1;
				parts[partCount] = dPtr;
				partCount++;
				if (partCount > max_parts) 
					return OPDI_ERROR_PARTS_OVERFLOW;
				// start at the beginning of the new string
				dPos = 0;
			}
		} else {
			// non-separator char found
			// leave out whitespace at the beginning of the string if specified
			if (!(dPos == 0 && trim && opdi_is_space(str[sPos]))) {
				dPtr[dPos] = str[sPos];
				dPos++;
			}
		}
		sPos++;
	}

	if (trim)
		// trim the ends of each part
		for (i = 0; i < partCount - 1; i++) {
			dPtr = (char *)parts[i];
			sPos = 0;
			dPos = 0;
			while (dPtr[sPos]) {
				if (!opdi_is_space(dPtr[sPos]))
					dPos = sPos;
				sPos++;
			}
			// dPos contains the position of the last non-space character
			dPtr[dPos + 1] = '\0';
		}
	// clear remaining parts
	for (i = partCount; i < max_parts; i++)
		parts[i] = NULL;
	if (part_count != NULL)
		*part_count = partCount;

	return OPDI_STATUS_OK;
}


uint8_t strings_join(const char **parts, char separator, char *dest, uint16_t max_length) {
	const char *part;
	uint8_t curPart = 0;
	uint16_t dPos = 0;
	uint16_t sPos = 0;

	// loop through parts
	while ((part = parts[curPart]) != NULL) {
		sPos = 0;
		// empty string?
		if (part[sPos] == '\0') {
			dest[dPos++] = ' ';	// use a blank
		} else {
			// loop over characters
			while (part[sPos]) {
				dest[dPos++] = part[sPos];
				// escape separator in parts
				if (part[sPos] == separator)
					dest[dPos++] = separator;
				if (dPos >= max_length - 1)
					return OPDI_ERROR_STRINGS_OVERFLOW;
				sPos++;
			}
		}
		if (dPos >= max_length - 1)
			return OPDI_ERROR_STRINGS_OVERFLOW;
		curPart++;
		if (parts[curPart] != NULL)
			dest[dPos++] = separator;
	}
	dest[dPos] = '\0';
	return OPDI_STATUS_OK;
}


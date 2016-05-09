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

// Windows specific functions

// disable conversion function deprecation warnings
#define _CRT_SECURE_NO_WARNINGS	1

#include <stdlib.h>
#include <windows.h>

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

uint8_t opdi_str_to_uint8(const char *str, uint8_t *result) {
	char *p;
	long res;

	// clear error
	errno = 0;

	res = strtol(str, &p, 10);

	if (errno != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = (uint8_t)res;
	return OPDI_STATUS_OK;
}

uint8_t opdi_str_to_uint16(const char *str, uint16_t *result) {
	char *p;
	long res;

	// clear error
	errno = 0;

	res = strtol(str, &p, 10);

	if (errno != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = (uint16_t)res;
	return OPDI_STATUS_OK;
}

uint8_t opdi_str_to_int32(const char *str, int32_t *result) {
	char *p;
	long res;

	// clear error
	errno = 0;

	res = strtol(str, &p, 10);

	if (errno != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = res;
	return OPDI_STATUS_OK;
}


uint8_t opdi_str_to_int64(const char *str, int64_t *result) {
	char *p;
	long res;

	// clear error
	errno = 0;

	res = strtol(str, &p, 10);

	if (errno != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = res;
	return OPDI_STATUS_OK;
}

uint8_t opdi_uint8_to_str(uint8_t value, char* msgBuf) {
	_itoa(value, msgBuf, 10);
	return (uint8_t)strlen(msgBuf);
}

uint8_t opdi_uint16_to_str(uint16_t value, char* msgBuf) {
	_itoa(value, msgBuf, 10);
	return (uint8_t)strlen(msgBuf);
}

uint8_t opdi_int32_to_str(int32_t value, char* msgBuf) {
	_itoa(value, msgBuf, 10);
	return (uint8_t)strlen(msgBuf);
}

uint8_t opdi_int64_to_str(int64_t value, char* msgBuf) {
	_i64toa(value, msgBuf, 10);
	return (uint8_t)strlen(msgBuf);
}

uint8_t opdi_bytes_to_string(uint8_t bytes[], uint16_t offset, uint16_t length, char *dest, uint16_t dest_length) {
	uint16_t i;
	// supports only single-byte character sets
	for (i = 0; i < length; i++) {
		if (i >= dest_length - 1)
			return OPDI_ERROR_DEST_OVERFLOW;
		dest[i] = bytes[offset + i];
	}
	dest[i] = '\0';
	return OPDI_STATUS_OK;
}

uint8_t opdi_string_to_bytes(char* string, uint8_t *dest, uint16_t pos, uint16_t maxlength, uint16_t *bytelen) {
	uint16_t i = 0;
	// supports only single-byte character sets
	while (string[i]) {
		if (pos + i >= maxlength)
			return OPDI_ERROR_DEST_OVERFLOW;
		dest[pos + i] = string[i];
		i++;
	}
	*bytelen = i;
	return OPDI_STATUS_OK;
}

uint8_t opdi_is_space(char c) {
	return iswspace(c);
}

int8_t opdi_string_cmp(const char *s1, const char *s2) {
	int result = _stricmp(s1, s2);
	if (result < 0)
		return -1;
	else
	if (result > 0)
		return 1;
	return 0;
}

uint64_t opdi_get_time_ms(void) {
   	return GetTickCount();
}

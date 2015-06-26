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
    
// AVR 8 bit specific functions

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

// strtoll implementation:
/*-
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Minimum and maximum values a `signed int64_t int' can hold.  */
#ifndef LLONG_MAX
#   define LLONG_MAX    9223372036854775807LL
#endif

#ifndef LLONG_MIN
#   define LLONG_MIN    (-LLONG_MAX - 1LL)
#endif
/* Maximum value an `uint64_t' can hold.  (Minimum is 0.)  */
#ifndef ULLONG_MAX
#   define ULLONG_MAX   18446744073709551615ULL
#endif

/*
 * Convert a string to a int64_t integer.
 *
 * Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
int64_t
strtoll(const char * nptr, char ** endptr, int base, uint8_t *errno)
{
        const char *s;
        uint64_t acc;
        char c;
        uint64_t cutoff;
        int neg, any, cutlim;

        /*
         * Skip white space and pick up leading +/- sign if any.
         * If base is 0, allow 0x for hex and 0 for octal, else
         * assume decimal; if base is already 16, allow 0x.
         */
        s = nptr;
        do {
                c = *s++;
        } while (isspace((unsigned char)c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X') &&
            ((s[1] >= '0' && s[1] <= '9') ||
            (s[1] >= 'A' && s[1] <= 'F') ||
            (s[1] >= 'a' && s[1] <= 'f'))) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;
        acc = any = 0;
        if (base < 2 || base > 36)
                goto noconv;

        /*
         * Compute the cutoff value between legal numbers and illegal
         * numbers.  That is the largest legal value, divided by the
         * base.  An input number that is greater than this value, if
         * followed by a legal input character, is too big.  One that
         * is equal to this value may be valid or not; the limit
         * between valid and invalid numbers is then based on the last
         * digit.  For instance, if the range for quads is
         * [-9223372036854775808..9223372036854775807] and the input base
         * is 10, cutoff will be set to 922337203685477580 and cutlim to
         * either 7 (neg==0) or 8 (neg==1), meaning that if we have
         * accumulated a value > 922337203685477580, or equal but the
         * next digit is > 7 (or 8), the number is too big, and we will
         * return a range error.
         *
         * Set 'any' if any `digits' consumed; make it negative to indicate
         * overflow.
         */
        cutoff = neg ? (uint64_t)-(LLONG_MIN + LLONG_MAX) + LLONG_MAX
            : LLONG_MAX;
        cutlim = cutoff % base;
        cutoff /= base;
        for ( ; ; c = *s++) {
                if (c >= '0' && c <= '9')
                        c -= '0';
                else if (c >= 'A' && c <= 'Z')
                        c -= 'A' - 10;
                else if (c >= 'a' && c <= 'z')
                        c -= 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
                        any = -1;
                else {
                        any = 1;
                        acc *= base;
                        acc += c;
                }
        }
        if (any < 0) {
                acc = neg ? LLONG_MIN : LLONG_MAX;
                *errno = -1;
        } else if (!any) {
noconv:
                *errno = -2;
        } else if (neg)
                acc = -acc;
        if (endptr != NULL)
                *endptr = (char *)(any ? s - 1 : nptr);
        return (acc);
}
// end of strtoll implementation

uint8_t opdi_str_to_uint8(const char *str, uint8_t *result) {
	static uint8_t errno;
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

uint8_t opdi_str_to_uint16(const char *str, uint16_t *result) {
	static uint8_t errno;
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

uint8_t opdi_str_to_int32(const char *str, int32_t *result) {
	static uint8_t errno;
	char *p;
	int32_t res;

	// clear error
	errno = 0;

	res = strtol(str, &p, 10);

	if (errno != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = res;
	return OPDI_STATUS_OK;
}

uint8_t opdi_str_to_int64(const char *str, int64_t *result) {
	uint8_t  error;
	char *p;
	int64_t res;

	// clear error
	error = 0;

	res = strtoll(str, &p, 10, &error);

	if (error != 0 || *p != 0 || p == str)
	  return OPDI_ERROR_CONVERSION;

	*result = res;
	return OPDI_STATUS_OK;
}

uint8_t opdi_uint8_to_str(uint8_t value, char* msgBuf) {
	itoa(value, msgBuf, 10);
	return strlen(msgBuf);
}

uint8_t opdi_uint16_to_str(uint16_t value, char* msgBuf) {
	itoa(value, msgBuf, 10);
	return strlen(msgBuf);
}

uint8_t opdi_int32_to_str(int32_t value, char* msgBuf) {
	ltoa(value, msgBuf, 10);
	return strlen(msgBuf);
}

uint8_t opdi_int64_to_str(int64_t n, char* pStr) {
	// code copied from: http://www.hlevkin.com/C_progr/long64.c
  int i = 0;
  int m;
  int len;
  char c;
  char s = '+';

 // if(n == LONG_LONG_MIN) // _I64_MIN  for Windows Microsoft compiler
  if(n < -9223372036854775807)
  {
    strcpy(pStr,"-9223372036854775808");
    return strlen(pStr);
  }

  if( n < 0 )
  {
    s = '-';
    n = - n;
    pStr[0]='-';
    i++;
  }

  do
  {
    m = n % (int64_t)10;
    pStr[i] = '0'+ m;
    n = n / (int64_t)10;
    i++;
  }
  while(n != 0);

  if(s == '+')
  {
    len = i;
  }
  else /* s=='-' */
  {
    len = i-1;
    pStr++;
  }

  for(i=0; i<len/2; i++)
  {
    c = pStr[i];
    pStr[i]       = pStr[len-1-i];
    pStr[len-1-i] = c;
  }
  pStr[len] = 0;

  if(s == '-')
  {
    pStr--;
  }
  return strlen(pStr);
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
	return isspace(c);
}

uint8_t opdi_string_cmp(const char *s1, const char *s2) {
	return strcmp(s1, s2);
}


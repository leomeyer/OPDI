
/************************************************************************/
/*                      AVRTools                                        */
/*                      Formatting routines								*/
/*																		*/
/*                      Version 0.1                                     */
/* Code License: GNU GPL												*/
/************************************************************************/
// Leo Meyer, 11/2009

#ifndef WIN32
#include <stdbool.h>
#endif
#include "format.h"

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

uint8_t fstr_dec_f(const int32_t val, uint8_t prec, uint8_t dec, char sep, char *dest, uint8_t flags) {
	
	int32_t h;				// helper
	int8_t mdec;			// number of decimals still to display
	bool sign;				// sign flag
	int32_t mval;			// current value
	int8_t pos;				// insert position
	bool leadZero = false;		// flag: insert leading zero
	char fillChar = (((flags && FORMAT_FILLZEROES) == FORMAT_FILLZEROES) ? '0' : ' ');

	if (val < 0) {
		sign = true;
		mval = val * -1;
	} else {
		sign = false;
		mval = val;
	}

	// determine rightmost insert position
	pos = prec 		// digits before sep
			+ 1 	// trailing '\0'
			+ (dec > 0 ? dec + 1 : 0);		// dec plus sep
	if (pos < 1) return 2;		// error
	
	// string end marker
	dest[pos--] = '\0';
	
	// no decimals? ignore separator
	if (dec == 0) 
		mdec = -1;
	else
		mdec = dec;
	// no value? show zero
	if (val == 0) 
		leadZero = 1;

	// iterate from the end
	while (pos >= 0) {
		if (mval > 0) {
			// decimals left?
			if (mdec > 0) {		
				h = mval % 10;
				mval /= 10;
				dest[pos--] = ('0' + h);
				mdec--;
			} else if (mdec == 0) {
				dest[pos--] = sep;
				mdec--;
			} else {
				// decimals finished
				h = mval % 10;
				mval /= 10;
				dest[pos--] = ('0' + h);
			}
		} else if (mval == 0) {		// last digit shown?
			if (mdec > 0) {
				// decimal zeroes left
				dest[pos--] = '0';
				mdec--;
			} else if (mdec == 0) {
				dest[pos--] = sep;
				mdec--;
				leadZero = 1;
			} else if (leadZero) {
				dest[pos--] = '0';
				leadZero = false;
			} else if (sign) {
				// display the sign next
				// or fill with zeroes
				if ((pos > 0) && ((flags && FORMAT_FILLZEROES) == FORMAT_FILLZEROES)) {
					dest[pos--] = fillChar;
				} else {
					dest[pos--] = '-';
					sign = false;	// clear sign flag, fill rest with blanks
				}
			} else {
				dest[pos--] = fillChar;
				mval--;
			}
		} else {
			// all digits displayed
			// fill with blanks
			dest[pos--] = fillChar;
		}
	}
	// overflow detection
	if (mdec > 0) {
		// remaining decimals don't fit
		dest[sign] = '*';
		return 1;
	} else if (leadZero) {
		// remaining leading zero does not fit
		dest[sign] = '*';
		return 1;
	} else if (mval > 0) {
		// remaining value does not fit
		if (sign) {
			dest[0] = '-';
		}
		dest[sign] = '*';
		return 1;
	} else if (mval == 0) {
		if (sign) {
			// remaining sign doesn't fit
			dest[0] = '-';
			dest[sign] = '*';
		}
		return 1;
	}
	// everything ok	
	return 0;
}


uint8_t fstr_dec(const int32_t val, uint8_t prec, uint8_t dec, char sep, char *dest) {
	return fstr_dec_f(val, prec, dec, sep, dest, FORMAT_STANDARD);
}


int32_t fstr2int(char* buffer, char sep, int8_t *result) {

	int32_t ret = 0;
	int8_t i = 0;
	uint8_t pow = 0;
	char c;
	bool sign = false;
	uint8_t presep = 0;
	int8_t postsep = -1;

	const int32_t powersOf10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
	
	// validate
	c = buffer[i];
	while (c) {
		if (c == '-') {
			if (sign) {
				// error: too many signs
				*result = -1;
				return 0;
			}
		} else if (c == sep) {
			if (postsep >= 0) {
				// error: too many separators
				*result = -2;
				return 0;
			} else {
				// first sep sets postsep to 0
				postsep = 0;
			}
		} else if (c == ' ') {
			// blanks are allowed only before the number
			if (sign) {
				// error: whitespace after sign
				*result = -5;
				return 0;
			}
			if ((presep > 0) || (postsep >= 0)) {
				// error: whitespace after digits
				*result = -5;
				return 0;
			}
		} else if ((c < '0') || (c > '9')) {
			// error: wrong character
			*result = -2;
			return 0;
		} else {
			// valid digit
			// after sep?
			if (postsep >= 0)
				postsep++;
			else
				presep++;
		}
		c = buffer[++i];
	}
	if (presep + postsep == 0) {
		// buffer contains no digits
		*result = -6;
		return 0;		
	}
	if (postsep == 0) {
		// buffer ends with sep
		*result = -7;
		return 0;
	}
	// no digits after sep?
	if (postsep < 0) postsep = 0;
	// parse
	*result = postsep;	// assume it's ok
	
	// set initial position
	i -= 1;	// last character
	while (i >= 0) {
		c = buffer[i];
		if (c == sep) {
			// nothing
		} else if (c == '-') {
			return -ret;			
		} else if (c == ' ') {
			return ret;
		} else {		
			ret += (c - '0') * powersOf10[pow];
			pow++;
			if (pow > 9) {		// length(powersOf10) - 1
				// too many digits, overflow
				*result = -8;
				return 0;
			}
		}
		i--;
	}
	return ret;
}
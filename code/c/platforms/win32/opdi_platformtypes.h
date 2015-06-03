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
    
// Windows type definitions

#ifndef __OPDI_PLATFORMTYPES_H
#define __OPDI_PLATFORMTYPES_H

// basic types on Windows

#define uint8_t		unsigned char
#define int8_t		signed char
#define uint16_t	unsigned __int16
#define int16_t		signed __int16
#define uint32_t	unsigned __int32
#define int32_t		signed __int32
#define uint64_t	unsigned __int64
#define int64_t		__int64

// channel number data type bit length
// It's important to define it this way because the C preprocessor can't compare strings.
// Way down in the protocol implementation this information is needed again.
#define channel_bits	16

#if (channel_bits == 8)
	#define channel_t	uint8_t
#elif (channel_bits == 16)
	#define channel_t	uint16_t
#else
#error "Not implemented; please define channel number data type"
#endif


#endif		// __OPDI_PLATFORMTYPES_H
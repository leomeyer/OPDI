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

// Arduino configuration specifications for DoorControl project

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#define OPDI_IS_SLAVE	1
#define OPDI_ENCODING_DEFAULT	OPDI_ENCODING_UTF8

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in data and the same amount on the stack.
#define OPDI_MESSAGE_BUFFER_SIZE		50

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in data and the same amount on the stack.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be OPDI_MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	8

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	1

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	8

// define to conserve RAM and ROM
//#define OPDI_NO_DIGITAL_PORTS

// define to conserve RAM and ROM
// Arduino compiler may crash or loop endlessly if this defined, so better keep them
//#define OPDI_NO_ANALOG_PORTS

// define to conserve RAM and ROM
#define OPDI_NO_SELECT_PORTS

// define to conserve RAM and ROM
// #define OPDI_NO_DIAL_PORTS

// maximum number of streaming ports
// set to 0 to conserve both RAM and ROM
#define OPDI_STREAMING_PORTS		0

// Define to conserve memory
#define OPDI_NO_ENCRYPTION

// Define to conserve memory
// #define OPDI_NO_AUTHENTICATION

#define OPDI_HAS_MESSAGE_HANDLED

// Defines for the OPDI implementation on Arduino

// keep these numbers as low as possible to conserve memory
#define MAX_PORTIDLENGTH		2
#define MAX_PORTNAMELENGTH		5
#define MAX_SLAVENAMELENGTH		12
#define MAX_ENCODINGNAMELENGTH	7

#define OPDI_MAX_PORT_INFO_MESSAGE	0

#define OPDI_EXTENDED_INFO_LENGTH	16

#define OPDI_EXTENDED_PROTOCOL	1

#define OPDI_EXTENDED_INFO_LENGTH	1

#endif		// __OPDI_CONFIGSPECS_H

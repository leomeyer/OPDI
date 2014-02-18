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
    
// WizChek configuration specifications

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#ifdef __cplusplus
extern "C" {
#endif

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in RAM.
#define OPDI_MESSAGE_BUFFER_SIZE		35

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in RAM.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be OPDI_MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	7

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	12

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	2

// define to conserve RAM and ROM
#define OPDI_NO_SELECT_PORTS

// define to conserve RAM and ROM
#define OPDI_NO_DIAL_PORTS

// maximum number of streaming ports
// set to 0 to conserve both RAM and ROM
#define OPDI_STREAMING_PORTS		0

// Define to conserve memory
#define OPDI_NO_ENCRYPTION

// Define to conserve memory
#define OPDI_NO_AUTHENTICATION

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_CONFIGSPECS_H
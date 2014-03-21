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
    
// Common OPDI constants

#ifndef __OPDI_CONSTANTS_H
#define __OPDI_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif 

// common status codes
#define OPDI_STATUS_OK					0
#define OPDI_DISCONNECTED				1
#define OPDI_TIMEOUT					2
#define	OPDI_CANCELLED					3
#define OPDI_ERROR_MALFORMED_MESSAGE	4
#define OPDI_ERROR_CONVERSION			5
#define OPDI_ERROR_MSGBUF_OVERFLOW		6
#define OPDI_ERROR_DEST_OVERFLOW		7
#define OPDI_ERROR_STRINGS_OVERFLOW		8
#define OPDI_ERROR_PARTS_OVERFLOW		9
#define OPDI_PROTOCOL_ERROR				10
#define OPDI_PROTOCOL_NOT_SUPPORTED		11
#define OPDI_ENCRYPTION_NOT_SUPPORTED	12
#define OPDI_ENCRYPTION_REQUIRED		13
#define OPDI_ENCRYPTION_ERROR			14
#define OPDI_AUTH_NOT_SUPPORTED			15
#define	OPDI_AUTHENTICATION_EXPECTED	16
#define OPDI_AUTHENTICATION_FAILED		17
#define OPDI_DEVICE_ERROR				18
#define OPDI_TOO_MANY_PORTS				19
#define OPDI_PORTTYPE_UNKNOWN			20
#define OPDI_PORT_UNKNOWN				21
#define OPDI_WRONG_PORT_TYPE			22
#define OPDI_TOO_MANY_BINDINGS			23
#define OPDI_NO_BINDING					24
#define OPDI_CHANNEL_INVALID			25
#define OPDI_POSITION_INVALID			26
#define OPDI_NETWORK_ERROR				27
#define OPDI_TERMINATOR_IN_PAYLOAD		28

// encodings
#define OPDI_ENCODING_ASCII			0
#define	OPDI_ENCODING_ISO8859_1		1
#define	OPDI_ENCODING_UTF8			2

#define OPDI_DONT_USE_ENCRYPTION	0
#define OPDI_USE_ENCRYPTION			1

// The default timeout for messages in milliseconds.
// May not exceed 65535.
#define OPDI_DEFAULT_MESSAGE_TIMEOUT		10000

// constants for the receive function
#define OPDI_CANNOT_SEND	0
#define OPDI_CAN_SEND		1

// constants for the debug message
// this message has been received from the master
#define OPDI_DIR_INCOMING	0
// this message is being sent to the master
#define OPDI_DIR_OUTGOING	1
// this message is a debug message from the master, sent on the control channel
#define OPDI_DIR_DEBUG	2

// Timeout for authentication (milliseconds).
// To give the user time to enter credentials.
// May not exceed 65535.
#define OPDI_AUTHENTICATION_TIMEOUT	60000

// Defines the connection state as provided to the ProtocolCallback function.
#define OPDI_PROTOCOL_START_HANDSHAKE	0
#define OPDI_PROTOCOL_CONNECTED			1
#define OPDI_PROTOCOL_DISCONNECTED		2

// Device flag constants
/** Is used to indicate that this device must use encryption. 
*   The device may choose one of the supported encryptions. 
*/
#define OPDI_FLAG_ENCRYPTION_REQUIRED		0x01

/** Is used to indicate that this device may not use encryption. 
*/
#define OPDI_FLAG_ENCRYPTION_NOT_ALLOWED	0x02

/** Is used to indicate that this device requires authentication. 
*   Authentication without encryption causes the password to be sent in plain text! 
*/
#define OPDI_FLAG_AUTHENTICATION_REQUIRED	0x04

// Port flag constants

#define OPDI_DIGITAL_PORT_HAS_PULLUP		0x01
#define OPDI_DIGITAL_PORT_HAS_PULLDN		0x02
#define OPDI_DIGITAL_PORT_PULLUP_ALWAYS 	0x04
#define OPDI_DIGITAL_PORT_PULLDN_ALWAYS		0x08

#define OPDI_ANALOG_PORT_CAN_CHANGE_RES		0x01
#define OPDI_ANALOG_PORT_RESOLUTION_8		0x02
#define OPDI_ANALOG_PORT_RESOLUTION_9		0x04
#define OPDI_ANALOG_PORT_RESOLUTION_10		0x08
#define OPDI_ANALOG_PORT_RESOLUTION_11		0x10
#define OPDI_ANALOG_PORT_RESOLUTION_12		0x20
#define OPDI_ANALOG_PORT_CAN_CHANGE_REF		0x200
#define OPDI_ANALOG_PORT_REFERENCE_INT		0x400
#define OPDI_ANALOG_PORT_REFERENCE_EXT		0x800

// streaming ports
#define OPDI_STREAMING_PORT_NORMAL			0
#define OPDI_STREAMING_PORT_AUTOBIND		1		// specifies that the master may automatically bind on connect

// port state constants

#define OPDI_DIGITAL_MODE_INPUT_FLOATING	"0"
#define OPDI_DIGITAL_MODE_INPUT_PULLUP		"1"
#define OPDI_DIGITAL_MODE_INPUT_PULLDOWN	"2"
#define OPDI_DIGITAL_MODE_OUTPUT			"3"

#define OPDI_DIGITAL_LINE_LOW				"0"
#define OPDI_DIGITAL_LINE_HIGH				"1"

#ifdef __cplusplus
}
#endif

#endif

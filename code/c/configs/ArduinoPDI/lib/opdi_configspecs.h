// libwizduino configuration specifications

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#define OPDI_IS_SLAVE	1

#define OPDI_ENCODING_DEFAULT	OPDI_ENCODING_UTF8

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in data and the same amount on the stack.
#define OPDI_MESSAGE_BUFFER_SIZE		48

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in data and the same amount on the stack.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be OPDI_MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	12

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	32

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	8

// define to conserve RAM and ROM
//#define OPDI_NO_DIGITAL_PORTS

// define to conserve RAM and ROM
//#define OPDI_NO_ANALOG_PORTS

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

#define OPDI_HAS_MESSAGE_HANDLED

// Defines for the OPDI implementation on Arduino

// keep these numbers as low as possible to conserve memory
#define MAX_PORTIDLENGTH		5
#define MAX_PORTNAMELENGTH		20
#define MAX_SLAVENAMELENGTH		16
#define MAX_ENCODINGNAMELENGTH	10

#endif		// __OPDI_CONFIGSPECS_H

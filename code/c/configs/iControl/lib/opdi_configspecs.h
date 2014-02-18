// libwizslave configuration specifications
// Include this file in your AVR Studio project to gain access
// to the device specific settings.

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#ifdef __cplusplus
extern "C" {
#endif

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in data and the same amount on the stack.
#define OPDI_MESSAGE_BUFFER_SIZE		512

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in data and the same amount on the stack.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be OPDI_MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	16

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	32

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	32

// define to conserve RAM and ROM
//#define OPDI_NO_SELECT_PORTS

// define to conserve RAM and ROM
//#define OPDI_NO_DIAL_PORTS

// maximum number of streaming ports
// set to 0 to conserve both RAM and ROM
#define OPDI_STREAMING_PORTS		4

// Define to conserve memory
//#define OPDI_NO_ENCRYPTION

// Define to conserve memory
//#define OPDI_NO_AUTHENTICATION

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_CONFIGSPECS_H
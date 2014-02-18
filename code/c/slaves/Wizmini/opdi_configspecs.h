// Wizmini configuration specifications

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#ifdef __cplusplus
extern "C" {
#endif

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in data and the same amount on the stack.
#define OPDI_MESSAGE_BUFFER_SIZE		40

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in data and the same amount on the stack.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be OPDI_MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	8

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	12

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	4

// maximum number of streaming ports
// set to 0 to conserve both RAM and ROM
#define OPDI_STREAMING_PORTS		0

// define to conserve RAM and ROM
//#define OPDI_NO_DIGITAL_PORTS

// define to conserve RAM and ROM
//#define OPDI_NO_ANALOG_PORTS

// Define to conserve memory
#define OPDI_NO_SELECT_PORTS

// Define to conserve memory
#define OPDI_NO_DIAL_PORTS

// Define to conserve memory
#define OPDI_NO_ENCRYPTION

// Define to conserve memory
#define	OPDI_NO_AUTHENTICATION

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_CONFIGSPECS_H
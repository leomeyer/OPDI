// Poladisc configuration specifications

#ifndef __OPDI_CONFIGSPECS_H
#define __OPDI_CONFIGSPECS_H

#ifdef __cplusplus
extern "C" {
#endif

#define OPDI_IS_SLAVE	1

// Defines the maximum message length this slave can receive.
// Consumes this amount of bytes in data and the same amount on the stack.
#define OPDI_MESSAGE_BUFFER_SIZE		48

// Defines the maximum message string length this slave can receive.
// Consumes this amount of bytes times sizeof(char) in data and the same amount on the stack.
// As the channel identifier and the checksum of a message typically consume a
// maximum amount of nine bytes, this value may be MESSAGE_BUFFER_SIZE - 9
// on systems with only single-byte character sets.
#define OPDI_MESSAGE_PAYLOAD_LENGTH	(OPDI_MESSAGE_BUFFER_SIZE - 9)

// maximum permitted message parts
#define OPDI_MAX_MESSAGE_PARTS	12

// maximum length of master's name this device will accept
#define OPDI_MASTER_NAME_LENGTH	1

#define OPDI_ENCODING_DEFAULT	OPDI_ENCODING_ISO8859_1

// maximum possible ports on this device
#define OPDI_MAX_DEVICE_PORTS	9

// Define to conserve memory
#define OPDI_NO_DIGITAL_PORTS

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
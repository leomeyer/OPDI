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

// Common device functions
//
// The functions and variables in this module must be provided by a config implementation.
// A configuration may choose to implement only slave or master functions or both.
// The subset that needs to be implemented depends on the defines in opdi_configspecs.h
// and can be limited to conserve resources.

#ifndef __OPDI_CONFIG_H
#define __OPDI_CONFIG_H

#include <stdlib.h>

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_port.h"

#ifdef __cplusplus
extern "C" {
#endif 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common properties of masters and slaves
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Flag combination that is valid for this configuration.
*   The meaning of the flags depends on whether this configuration acts as a master or a slave.
*/
extern uint16_t opdi_device_flags;

/** Used for debug messages. All sent or received messages are passed to this function.
*   This function must be provided by a config implementation. It may choose to return
*   only OPDI_STATUS_OK and do nothing else.
*   See OPDI_DIR_* constants in opdi_constants.h for valid values for direction.
*/
extern uint8_t opdi_debug_msg(const char *str, uint8_t direction);

// encryption may be disabled by setting this define
#ifndef OPDI_NO_ENCRYPTION

/** The encryption that should be used after the handshake. May not be empty or NULL. 
*   To use encryption you must also provide functions for encrypting and decrypting blocks.
*   You must also define ENCRYPTION_BLOCKSIZE in the config specs.
*/
extern char opdi_encryption_method[];

/** Is used to encrypt a block of bytes of size ENCRYTION_BLOCKSIZE.
*   dest and src are buffers of exactly this size.
*   Must be provided by the implementation.
*/
extern uint8_t opdi_encrypt_block(uint8_t *dest, const uint8_t *src);

/** Is used to decrypt a block of bytes of size ENCRYTION_BLOCKSIZE.
*   dest and src are buffers of exactly this size.
*   Must be provided by the implementation.
*/
extern uint8_t opdi_decrypt_block(uint8_t *dest, const uint8_t *src);

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Slave functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef OPDI_IS_SLAVE

/** Callback function codes */
typedef enum {
	OPDI_FUNCTION_GET_CONFIG_NAME,					/** The name of the configuration. */
	OPDI_FUNCTION_SET_MASTER_NAME,					/** Called when a connecting master's name has been received. */
	OPDI_FUNCTION_GET_SUPPORTED_PROTOCOLS,			/** A comma-separated list of supported protocols. */
	OPDI_FUNCTION_GET_ENCODING,						/** The encoding that should be used. One of the encoding identifiers defined in the class java.lang.Charset. May be empty. */
	OPDI_FUNCTION_SET_LANGUAGES,					/** Passes a string of comma-separated preferred languages as received from the master. The language names correspond to the predefined constants in the class java.util.Locale. */
	OPDI_FUNCTION_GET_EXTENDED_DEVICEINFO,			/** Returns a key=value;-string describing extended device information. May be empty. */
	OPDI_FUNCTION_GET_EXTENDED_PORTINFO,			/** Returns a key=value;-string describing extended pprt information. The buffer contains the port ID. May be empty. */
	OPDI_FUNCTION_GET_EXTENDED_PORTSTATE			/** Returns a key=value;-string describing the extended port state. The buffer contains the port ID. May be empty. */

#ifndef OPDI_NO_AUTHENTICATION
	, OPDI_FUNCTION_SET_USERNAME					/** During authentication, first the username is set. */
	, OPDI_FUNCTION_SET_PASSWORD					/** Next, the password is set. If the implementation returns any code other than OPDI_STATUS_OK it is interpreted as "authentication failed". */
#endif
} OPDIFunctionCode;

/** The slave callback function that is used by the OPDI system to send data to or request data from
* the slave implementation. This function exists mostly to allow memory-limited devices, such as Arduinos,
* to keep string constants in flash ROM rather than defining them as extern const char*s, which would require
* them to be kept in RAM.
* An opdiFunctionCode can be either a SET or a GET code. If the code is SET the buffer or data fields will
* contain the information to transfer (depending on the code). If the code is GET the buffer may be filled
* by the implementation to transfer the values back to the OPDI system. In this case data may contain the
* maximum number of bytes to copy.
*/
extern uint8_t opdi_slave_callback(OPDIFunctionCode opdiFunctionCode, char *buffer, size_t data);


#ifdef OPDI_HAS_MESSAGE_HANDLED

/** Is called by the protocol when a message has been successfully processed. This means the master has been active.
*   This function can be used to implement an idle timer. This function should return OPDI_STATUS_OK to indicate that everything is ok.
*   It is usually ok to send messages from this function.
*/
extern uint8_t opdi_message_handled(channel_t channel, const char **parts);

#endif

#ifndef OPDI_NO_ANALOG_PORTS

/** Returns the state of an analog port.
*   For the values, see the Java AnalogPort implementation.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value);

/** Sets the value of an analog port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value);

/** Sets the mode of an analog port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]);

/** Sets the resolution of an analog port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]);

/** Sets the reference of an analog port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]);

#endif

#ifndef OPDI_NO_DIGITAL_PORTS

/** Returns the state of a digital port.
*   For the values, see the Java DigitalPort implementation.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]);

/** Sets the line state of a digital port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]);

/** Sets the mode of a digital port.
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]);

#endif

#ifndef OPDI_NO_SELECT_PORTS

/** Gets the state of a select port. 
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *position);

/** Sets the position of a select port. 
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position);

#endif

#ifndef OPDI_NO_DIAL_PORTS

/** Gets the state of a dial port. 
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_get_dial_port_state(opdi_Port *port, int64_t *position);

/** Sets the position of a dial port. 
*   This function is typically provided by a configuration.
*   Must return OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_set_dial_port_position(opdi_Port *port, int64_t position);

#endif

#endif		// OPDI_IS_SLAVE

#ifdef __cplusplus
}
#endif

#endif		// ndef __OPDI_CONFIG_H

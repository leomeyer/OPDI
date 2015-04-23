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

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_port.h"

#ifdef __cplusplus
extern "C" {
#endif 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common properties of masters and slaves
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** The name of the configuration. */
extern const char *opdi_config_name;

/** The encoding that should be used. One of the encoding identifiers defined in the class java.lang.Charset.
May be empty or NULL. */
extern const char *opdi_encoding;

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
extern const char *opdi_encryption_method;

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

/** A pointer to a buffer of at least length OPDI_MASTER_NAME_LENGTH that accepts the connecting master's name. */
extern char opdi_master_name[];

/** A comma-separated list of protocols supported by this slave. */
extern const char *opdi_supported_protocols;

#ifndef OPDI_NO_AUTHENTICATION

/** The username for authentication */
extern const char *opdi_username;
/** The password for authentication */
extern const char *opdi_password;

#endif

#ifdef OPDI_HAS_MESSAGE_HANDLED

/** Is called by the protocol when a message has been successfully processed. This means the master has been active.
*   This function can be used to implement an idle timer. This function should return OPDI_STATUS_OK to indicate that everything is ok.
*   It is usually ok to send messages from this function.
*/
extern uint8_t opdi_message_handled(channel_t channel, const char **parts);

#endif

/** Passes a string of comma-separated preferred languages as received from the master.
*   The language names correspond to the predefined constants in the class java.util.Locale.
*/
extern uint8_t opdi_choose_language(const char *languages);

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

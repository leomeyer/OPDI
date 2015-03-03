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
    
// OPDI slave protocol level functions

#ifndef __OPDI_SLAVE_PROTOCOL_H
#define __OPDI_SLAVE_PROTOCOL_H

#include "opdi_platformtypes.h"
#include "opdi_port.h"

#ifdef __cplusplus
extern "C" {
#endif 

/** Callback function that is called when the protocol status changes.
* Used by devices to update UI etc.
*/
typedef void (*opdi_ProtocolCallback)(uint8_t protocol_state);

/** Defines a protocol handler that has been identified by a protocol identifier.
*   This function typically runs a message processing loop.
*/
typedef uint8_t (*opdi_ProtocolHandler)(void);

/** Callback function to determine a protocol handler.
*   If this is NULL, supports only the basic protocol.
*   The protocol identifier from the master is passed into this function.
*   If this function returns NULL, the basic protocol handler is used.
*/
typedef opdi_ProtocolHandler (*opdi_GetProtocol)(const char *protocolID);

/** The message handler for the basic protocol.
*   This handler is available as a fallback mechanism for extended protocols.
*   It returns OPDI_STATUS_OK even if the message could not be processed (ignore unknown messages).
*/
//uint8_t opdi_handle_basic_message(opdi_Message *m);

/** Prepares the OPDI protocol implementation for a new connection.
*   Most importantly, this will disable encryption as it is not used at the beginning
*   of the handshake.
*/
uint8_t opdi_slave_init(void);

/** Starts the OPDI protocol by performing the handshake using the given message.
*   get_protocol is used to determine extended protocols. It is passed the master's chosen
*   protocol identifier and may return a protocol handler. It may also be NULL.
*   If it is NULL, only the basic protocol can be used.
*/
uint8_t opdi_slave_start(opdi_Message *m, opdi_GetProtocol get_protocol, opdi_ProtocolCallback protocol_callback);

/** Returns 1 if the slave is currently connected, i. e. a protocol handler is running; 0 otherwise.
*/
uint8_t opdi_slave_connected(void);

/** Sends a debug message to the master.
*/
uint8_t opdi_send_debug(const char *debugmsg);
	
/** Causes the Reconfigure message to be sent which prompts the master to re-read the device capabilities.
*/
uint8_t opdi_reconfigure(void);

/** Causes the Refresh message to be sent for the specified ports. The last element must be NULL.
*   If the first element is NULL, sends the empty refresh message causing all ports to be
*   refreshed.
*/
uint8_t opdi_refresh(opdi_Port **ports);

/** Causes the Disconnect message to be sent to the master.
*   Returns OPDI_DISCONNECTED. After this, no more messages may be sent to the master.
*/
uint8_t opdi_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif		// __OPDI_SLAVE_PROTOCOL_H

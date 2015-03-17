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
    
// Port functions

#ifndef __OPDI_PORT_H
#define __OPDI_PORT_H

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_message.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define OPDI_Q(x) #x
#define OPDI_QUOTE(x) OPDI_Q(x)

/** Port type constants.
*   Compared using string comparison.
*/
#define OPDI_PORTTYPE_DIGITAL	"0"
#define OPDI_PORTTYPE_ANALOG	"1"
#define OPDI_PORTTYPE_SELECT	"2"
#define OPDI_PORTTYPE_DIAL		"3"
#define OPDI_PORTTYPE_STREAMING	"4"

/** Port direction constants. 
*/
#define OPDI_PORTDIRCAP_UNKNOWN	""
// the port is an input port from the slave's perspective
// i. e. peripherals provide input to the slave
#define OPDI_PORTDIRCAP_INPUT	"0"
// the port is an output port from the slave's perspective
// i. e. the slave controls peripherals with this port
#define OPDI_PORTDIRCAP_OUTPUT	"1"
// the port may work in both directions (but only one at a
// given time)
#define OPDI_PORTDIRCAP_BIDI	"2"

// port flag constants

/** Indicates that a port is not always present, for example if a menu structure
is implemented using dial ports. Accessing a temporary port that is not present in
the current configuration should yield an OPDI_PORT_ACCESS_DENIED error. */
#define OPDI_PORT_TEMPORARY					0x8000

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

#define OPDI_STREAMING_PORT_NORMAL			0x00
#define OPDI_STREAMING_PORT_AUTOBIND		0x01		// specifies that the master may automatically bind on connect

// port state constants

#define OPDI_DIGITAL_MODE_UNKNOWN			-1
#define OPDI_DIGITAL_MODE_INPUT_FLOATING	0
#define OPDI_DIGITAL_MODE_INPUT_PULLUP		1
#define OPDI_DIGITAL_MODE_INPUT_PULLDOWN	2
#define OPDI_DIGITAL_MODE_OUTPUT			3

#define OPDI_DIGITAL_LINE_UNKNOWN			-1
#define OPDI_DIGITAL_LINE_LOW				0
#define OPDI_DIGITAL_LINE_HIGH				1

/** Used to avoid pointer casts to integer and back. 
*/
typedef union opdi_PtrInt {
	void *ptr;
	int32_t i;
} opdi_PtrInt;

/** Defines a port. Is also a node of a linked list of ports.
*   The info member contains port specific information:
*     digital: flags as integer value
*     analog: flags as integer value
*     select: pointer to an array of char* that contains the position labels;
*             the last element of this array must be NULL
*     dial: pointer to an opdi_DialPortInfo structure
*     streaming: pointer to an opdi_StreamingPortInfo structure
*/
typedef struct opdi_Port {
	const char *id;				// the ID of the port
	const char *name;			// the name of the port
	const char *type;			// the type of the port
	const char *caps;			// capabilities (port type dependent)
	opdi_PtrInt info;			// pointer to additional info (port type dependent)
	struct opdi_Port *next;		// pointer to next port
} opdi_Port;

/** Info structure for dial ports.
*/
typedef struct opdi_DialPortInfo {
	int32_t min;		// minimum value
	int32_t max;		// maximum value
	int32_t step;		// step width
} opdi_DialPortInfo;

/** Receiving function for streaming ports.
*/
typedef uint8_t (*DataReceived)(opdi_Port *port, const char *data);

/** Info structure for streaming ports.
*/
typedef struct opdi_StreamingPortInfo {
	// ID of the driver used for this streaming port
	const char *driverID;
	// port flags
	uint16_t flags;
	// pointer to a function that receives incoming data
	DataReceived dataReceived;
	// the channel number which is > 0 if the port is bound
	channel_t channel;
} opdi_StreamingPortInfo;

/** Holds streaming port bindings.
*/
typedef struct opdi_StreamingPortBinding {
	channel_t channel;
	struct opdi_Port *port;
} opdi_StreamingPortBinding;

/** Clears the list of ports. This does not free the memory associated with the ports.
*   Resets all port bindings of streaming ports.
*/
uint8_t opdi_clear_ports(void);

/** Returns the start of the linked list of ports.
*   NULL if the port list is empty.
*/
opdi_Port *opdi_get_ports(void);

/** Returns the end of the linked list of ports.
*   NULL if the port list is empty.
*/
opdi_Port *opdi_get_last_port(void);

/** Adds a port to the list. Returns OPDI_STATUS_OK if everything is ok.
*/
uint8_t opdi_add_port(opdi_Port *port);

/** Returns the port identified by the given ID.
*   Returns NULL if the port can't be found.
*/
opdi_Port *opdi_find_port_by_id(const char *id);

#if (OPDI_STREAMING_PORTS > 0)

/** Binds the port to the specified channel. The port must be a streaming port.
*/
uint8_t opdi_bind_port(opdi_Port *port, channel_t channel);

/** Unbinds the port. The port must be a streaming port.
*/
uint8_t opdi_unbind_port(opdi_Port *port);

/** Tries to dispatch the message payload to a bound streaming port. If a streaming port is found,
*   its dataReceived function from its port information is called and its result returned.
*   If there is no port bound to this channel, returns OPDI_NO_BINDING.
*/
uint8_t opdi_try_dispatch_stream(opdi_Message *m);

/** Resets all bindings. This will usually be called when a new connection is being initiated.
*/
uint8_t opdi_reset_bindings(void);

/** Returns the number of currently bound streaming ports.
*/
uint16_t opdi_get_port_bind_count(void);

#endif

/** Used to set the port info message for OPDI_PORT_ACCESS_DENIED and OPDI_PORT_ERROR.
*   The message is copied into an internal buffer.
*/
void opdi_set_port_message(const char *message);

/** Used to get the port info message for OPDI_PORT_ACCESS_DENIED and OPDI_PORT_ERROR.
*/
const char *opdi_get_port_message(void);

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_PORT_H

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
    
// Port functions

#ifndef __OPDI_PORTS_H
#define __OPDI_PORTS_H

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_messages.h"

#ifdef __cplusplus
extern "C" {
#endif 

/** Port type constants.
*   Compared using string comparison; can be selectively disabled.
*/
#ifndef OPDI_NO_DIGITAL_PORTS
#define OPDI_PORTTYPE_DIGITAL	"0"
#endif
#ifndef OPDI_NO_ANALOG_PORTS
#define OPDI_PORTTYPE_ANALOG	"1"
#endif
#ifndef OPDI_NO_SELECT_PORTS
#define OPDI_PORTTYPE_SELECT	"2"
#endif
#ifndef OPDI_NO_DIAL_PORTS
#define OPDI_PORTTYPE_DIAL		"3"
#endif
#if (OPDI_STREAMING_PORTS > 0)
#define OPDI_PORTTYPE_STREAMING	"4"
#endif

/** Port direction constants. 
*/
// the port is an input port from the slave's perspective
// i. e. peripherals provide input to the slave
#define OPDI_PORTDIRCAP_INPUT	"0"
// the port is an output port from the slave's perspective
// i. e. the slave controls peripherals with this port
#define OPDI_PORTDIRCAP_OUTPUT	"1"
// the port may work in both directions (but only one at a
// given time)
#define OPDI_PORTDIRCAP_BIDI	"2"

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
extern uint8_t opdi_clear_ports(void);

/** Returns the start of the linked list of ports.
*   NULL if the port list is empty.
*/
extern opdi_Port *opdi_get_ports(void);

/** Returns the end of the linked list of ports.
*   NULL if the port list is empty.
*/
extern opdi_Port *opdi_get_last_port(void);

/** Adds a port to the list. Returns OPDI_STATUS_OK if everything is ok.
*/
extern uint8_t opdi_add_port(opdi_Port *port);

/** Returns the port identified by the given ID.
*   Returns NULL if the port can't be found.
*/
extern opdi_Port *opdi_find_port_by_id(const char *id);

#if (OPDI_STREAMING_PORTS > 0)

/** Binds the port to the specified channel. The port must be a streaming port.
*/
extern uint8_t opdi_bind_port(opdi_Port *port, channel_t channel);

/** Unbinds the port. The port must be a streaming port.
*/
extern uint8_t opdi_unbind_port(opdi_Port *port);

/** Tries to dispatch the message payload to a bound streaming port. If a streaming port is found,
*   its dataReceived function from its port information is called and its result returned.
*   If there is no port bound to this channel, returns OPDI_NO_BINDING.
*/
extern uint8_t opdi_try_dispatch_stream(opdi_Message *m);

/** Resets all bindings. This will usually be called when a new connection is being initiated.
*/
extern uint8_t opdi_reset_bindings(void);

/** Returns the number of currently bound streaming ports.
*/
extern uint16_t opdi_get_port_bind_count(void);

#endif

#ifdef __cplusplus
}
#endif


#endif		// __OPDI_PORTS_H

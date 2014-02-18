//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-14 Leo Meyer (leo@leomeyer.de)
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

// Implements the OPDI specification.
//
// Port support may be selectively disabled by using the defines OPDI_NO_ANALOG_PORTS,
// OPDI_NO_DIGITAL_PORTS, OPDI_NO_SELECT_PORTS, OPDI_NO_DIAL_PORTS, and 
// OPDI_STREAMING_PORTS to conserve memory.
//
// Supported protocols are: basic protocol, extended protocol.
// For the extended protocol, define EXTENDED_PROTOCOL in your configspecs.h.
// By default, only the basic protocol is supported.
    
#include <stdlib.h>
#include <string.h>

#include "opdi_constants.h"
#include "opdi_strings.h"
#include "opdi_messages.h"
#include "opdi_slave_protocol.h"
#include "opdi_device.h"
#include "opdi_platformfuncs.h"
#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"

#define PARTS_SEPARATOR	':'
#define MULTIMESSAGE_SEPARATOR	'\r'

#define handshake 				"OPDI"
#define handshake_version 		"0.1"

#define basic_protocol_magic 	"BP"
#define extended_protocol_magic	"EP"

// control channel message identifiers
#define error 					"Err"
#define disconnect 				"Dis"
#define refresh 				"Ref"
#define reconfigure 			"Reconf"
#define debug 					"Debug"

#define agreement 				"OK"
#define disagreement 			"NOK"

#ifndef OPDI_NO_AUTHENTICATION
#define auth 					"Auth"
#endif

// protocol message identifiers
#define getDeviceCaps  			"gDC"
#define getPortInfo  			"gPI"

#ifndef OPDI_NO_ANALOG_PORTS
#define analogPort  			"AP"
#define analogPortState  		"AS"
#define getAnalogPortState  	"gAS"
#define setAnalogPortValue  	"sAV"
#define setAnalogPortMode  		"sAM"
#define setAnalogPortResolution "sAR"
#define setAnalogPortReference  "sARF"
#endif

#ifndef OPDI_NO_DIGITAL_PORTS
#define digitalPort  			"DP"
#define digitalPortState  		"DS"
#define getDigitalPortState  	"gDS"
#define setDigitalPortLine  	"sDL"
#define setDigitalPortMode  	"sDM"
#endif

#ifndef OPDI_NO_SELECT_PORTS
#define selectPort  			"SLP"
#define getSelectPortLabel  	"gSL"
#define selectPortLabel  		"SL"
#define selectPortState  		"SS"
#define getSelectPortState  	"gSS"
#define setSelectPortPosition	"sSP"
#endif

#ifndef OPDI_NO_DIAL_PORTS
#define dialPort  				"DL"
#define dialPortState  			"DLS"
#define getDialPortState  		"gDLS"
#define setDialPortPosition  	"sDLP"
#endif

#if (OPDI_STREAMING_PORTS > 0)
#define streamingPort  			"SP"
#define bindStreamingPort  		"bSP"
#define unbindStreamingPort  	"uSP"
#endif

#ifdef EXTENDED_PROTOCOL
#define getAllPortStates		"gAPS"
#endif

// for splitting messages into parts
static const char *parts[OPDI_MAX_MESSAGE_PARTS];
// for assembling a payload
static char payload[OPDI_MESSAGE_PAYLOAD_LENGTH];

// expects a control message on channel 0
static uint8_t expect_control_message(const char **parts, uint8_t *partCount) {
	opdi_Message m;
	uint8_t result;

	result = opdi_get_message(&m, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK)
		return result;

	// message must be on control channel
	if (m.channel != 0)
		return OPDI_PROTOCOL_ERROR;

	result = strings_split(m.payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, partCount);
	if (result != OPDI_STATUS_OK)
		return result;

	// disconnect message?
	if (!strcmp(parts[0], disconnect))
		return OPDI_DISCONNECTED;

	// error message?
	if (!strcmp(parts[0], error))
		return OPDI_DEVICE_ERROR;

	return OPDI_STATUS_OK;
}

static uint8_t send_error(uint8_t code, const char *part1, const char *part2) {
	// send an error message on the control channel
	char buf[5];
	opdi_Message message;
	uint8_t result;

	opdi_uint8_to_str(code, buf);

	// join payload
	parts[0] = error;
	parts[1] = buf;
	parts[2] = part1;
	parts[3] = part2;
	parts[4] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	// send on the control channel
	message.channel = 0;
	message.payload = payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return code;
}

static uint8_t send_disagreement(channel_t channel, uint8_t code, const char *part1, const char *part2) {
	// send a disagreement message on the specified channel
	opdi_Message message;
	uint8_t result;

	// join payload
	parts[0] = disagreement;
	parts[1] = part1;
	parts[2] = part2;
	parts[3] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	message.channel = channel;
	message.payload = payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return code;
}

#if (OPDI_STREAMING_PORTS > 0) || !defined(OPDI_NO_AUTHENTICATION)
static uint8_t send_agreement(channel_t channel) {
	// send an agreement message on the specified channel
	opdi_Message message;
	uint8_t result;

	// join payload
	parts[0] = agreement;
	parts[1] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	message.channel = channel;
	message.payload = payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}
#endif

/** Common function: send the contents of the parts array on the specified channel.
*/
static uint8_t send_payload(channel_t channel) {
	opdi_Message message;
	uint8_t result;

	message.channel = channel;
	message.payload = payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

/** Common function: send the contents of the parts array on the specified channel.
*/
static uint8_t send_parts(channel_t channel) {
	uint8_t result;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	result = send_payload(channel);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

// device capabilities

// send a comma-separated list of port IDs
static uint8_t send_device_caps(channel_t channel) {
	opdi_Message message;
	uint8_t portCount = 0;
	const char *parts[OPDI_MAX_DEVICE_PORTS];
	char portCSV[OPDI_MESSAGE_PAYLOAD_LENGTH];
	opdi_Port *port;
	uint8_t result;

	// prepare list of device ports
	parts[0] = NULL;
	port = opdi_get_ports();
	while (port != NULL) {
		if (portCount >= OPDI_MAX_DEVICE_PORTS)
			return OPDI_TOO_MANY_PORTS;
		parts[portCount++] = port->id;
		port = port->next;
	}
	parts[portCount] = NULL;

	// join port IDs as comma separated list
	result = strings_join(parts, ',', portCSV, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	// join payload
	parts[0] = "BDC";
	parts[1] = portCSV;
	parts[2] = NULL;
	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	// send on the same channel
	message.channel = channel;
	message.payload = payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;
	
	return OPDI_STATUS_OK;
}

#ifndef OPDI_NO_DIGITAL_PORTS
static uint8_t send_digital_port_info(channel_t channel, opdi_Port *port) {
	char flagStr[10];

	// port info is flags
	opdi_int32_to_str(port->info.i, flagStr);

	// join payload
	parts[0] = digitalPort;	// port magic
	parts[1] = port->id;
	parts[2] = port->name;
	parts[3] = port->caps;
	parts[4] = flagStr;
	parts[5] = NULL;

	return send_parts(channel);
}
#endif

#ifndef OPDI_NO_ANALOG_PORTS
static uint8_t send_analog_port_info(channel_t channel, opdi_Port *port) {
	char flagStr[10];

	// port info is flags
	opdi_int32_to_str(port->info.i, flagStr);

	// join payload
	parts[0] = analogPort;	// port magic
	parts[1] = port->id;
	parts[2] = port->name;
	parts[3] = port->caps;
	parts[4] = flagStr;
	parts[5] = NULL;

	return send_parts(channel);
}
#endif

#ifndef OPDI_NO_SELECT_PORTS
static uint8_t send_select_port_info(channel_t channel, opdi_Port *port) {
	char **labels;
	uint16_t positions = 0;
	char buf[7];

	// port info is an array of char*
	labels = (char**)port->info.ptr;

	// count positions
	while (labels[positions])
		positions++;

	// convert positions to str
	opdi_uint16_to_str(positions, (char *)&buf);

	// join payload
	parts[0] = selectPort;	// port magic
	parts[1] = port->id;
	parts[2] = port->name;
	parts[3] = buf;
	parts[4] = "0";		// flags: reserved
	parts[5] = NULL;

	return send_parts(channel);
}
#endif

#ifndef OPDI_NO_DIAL_PORTS
static uint8_t send_dial_port_info(channel_t channel, opdi_Port *port) {
	char minbuf[10];
	char maxbuf[10];
	char stepbuf[10];

	opdi_DialPortInfo *dpi = (opdi_DialPortInfo *)port->info.ptr;
	// convert values to strings
	opdi_int32_to_str(dpi->min, (char *)&minbuf);
	opdi_int32_to_str(dpi->max, (char *)&maxbuf);
	opdi_int32_to_str(dpi->step, (char *)&stepbuf);

	// join payload
	parts[0] = dialPort;	// port magic
	parts[1] = port->id;
	parts[2] = port->name;
	parts[3] = minbuf;
	parts[4] = maxbuf;
	parts[5] = stepbuf;
	parts[6] = "0";		// flags: reserved
	parts[7] = NULL;

	return send_parts(channel);
}
#endif

#if (OPDI_STREAMING_PORTS > 0)
static uint8_t send_streaming_port_info(channel_t channel, opdi_Port *port) {
	char buf[7];

	opdi_StreamingPortInfo *spi = (opdi_StreamingPortInfo *)port->info.ptr;
	// convert flags to str
	opdi_uint16_to_str(spi->flags, (char *)&buf);

	// join payload
	parts[0] = streamingPort;	// port magic
	parts[1] = port->id;
	parts[2] = port->name;
	parts[3] = spi->driverID;
	parts[4] = buf;
	parts[5] = NULL;

	return send_parts(channel);
}
#endif

static uint8_t send_port_info(channel_t channel, opdi_Port *port) {

#ifndef OPDI_NO_DIGITAL_PORTS
	if (!strcmp(port->type, OPDI_PORTTYPE_DIGITAL)) {
		return send_digital_port_info(channel, port);
#else
	// "better keep it gramat."
	if (0) {
#endif
#ifndef OPDI_NO_ANALOG_PORTS
	} else if (!strcmp(port->type, OPDI_PORTTYPE_ANALOG)) {
		return send_analog_port_info(channel, port);
#endif
#ifndef OPDI_NO_SELECT_PORTS
	} else if (!strcmp(port->type, OPDI_PORTTYPE_SELECT)) {
		return send_select_port_info(channel, port);
#endif
#ifndef OPDI_NO_DIAL_PORTS
	} else if (!strcmp(port->type, OPDI_PORTTYPE_DIAL)) {
		return send_dial_port_info(channel, port);
#endif
#if (OPDI_STREAMING_PORTS > 0)
	} else if (!strcmp(port->type, OPDI_PORTTYPE_STREAMING)) {
		return send_streaming_port_info(channel, port);
#endif
	} else
		return OPDI_PORTTYPE_UNKNOWN;
}

/// analog port functions

#ifndef OPDI_NO_ANALOG_PORTS
static uint8_t get_analog_port_state(opdi_Port *port) {
	uint8_t result;
	char mode[] = " ";
	char res[] = " ";
	char ref[] = " ";
	int32_t value = 0;
	char valStr[10];

	if (strcmp(port->type, OPDI_PORTTYPE_ANALOG)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	result = opdi_get_analog_port_state(port, mode, res, ref, &value);
	if (result != OPDI_STATUS_OK)
		return result;

	opdi_int32_to_str(value, valStr);

	// join payload
	parts[0] = analogPortState;
	parts[1] = port->id;
	parts[2] = mode;
	parts[3] = ref;
	parts[4] = res;
	parts[5] = valStr;
	parts[6] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

static uint8_t send_analog_port_state(channel_t channel, opdi_Port *port) {
	uint8_t result = get_analog_port_state(port);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_payload(channel);
}

static uint8_t set_analog_port_value(channel_t channel, opdi_Port *port, const char *value) {
	int32_t val;
	uint8_t result;

	result = opdi_str_to_int32(value, &val);
	if (result != OPDI_STATUS_OK)
		return result;

	result = opdi_set_analog_port_value(port, val);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_analog_port_state(channel, port);
}

static uint8_t set_analog_port_mode(channel_t channel, opdi_Port *port, const char *mode) {
	uint8_t result;

	result = opdi_set_analog_port_mode(port, mode);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_analog_port_state(channel, port);
}

static uint8_t set_analog_port_resolution(channel_t channel, opdi_Port *port, const char *res) {
	uint8_t result;

	result = opdi_set_analog_port_resolution(port, res);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_analog_port_state(channel, port);
}

static uint8_t set_analog_port_reference(channel_t channel, opdi_Port *port, const char *ref) {
	uint8_t result;

	result = opdi_set_analog_port_reference(port, ref);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_analog_port_state(channel, port);
}
#endif

/// digital port functions

#ifndef OPDI_NO_DIGITAL_PORTS
// writes the digital port state to the parts array
static uint8_t get_digital_port_state(opdi_Port *port) {
	uint8_t result;
	char mode[] = " ";
	char line[] = " ";

	if (strcmp(port->type, OPDI_PORTTYPE_DIGITAL)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	result = opdi_get_digital_port_state(port, mode, line);
	if (result != OPDI_STATUS_OK)
		return result;

	// join payload
	parts[0] = digitalPortState;
	parts[1] = port->id;
	parts[2] = mode;
	parts[3] = line;
	parts[4] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

static uint8_t send_digital_port_state(channel_t channel, opdi_Port *port) {
	uint8_t result = get_digital_port_state(port);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_payload(channel);
}

static uint8_t set_digital_port_line(channel_t channel, opdi_Port *port, const char *line) {
	uint8_t result;

	result = opdi_set_digital_port_line(port, line);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_digital_port_state(channel, port);
}

static uint8_t set_digital_port_mode(channel_t channel, opdi_Port *port, const char *mode) {
	uint8_t result;

	result = opdi_set_digital_port_mode(port, mode);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_digital_port_state(channel, port);
}
#endif

/// select port functions

#ifndef OPDI_NO_SELECT_PORTS
static uint8_t send_select_port_label(channel_t channel, opdi_Port *port, const char *position) {
	uint8_t result;
	uint16_t pos;
	uint16_t i;
	char **labels;

	if (strcmp(port->type, OPDI_PORTTYPE_SELECT)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	// port info is an array of char*
	labels = (char**)port->info.ptr;

	result = opdi_str_to_uint16(position, &pos);
	if (result != OPDI_STATUS_OK)
		return result;

	// count labels up to
	i = 0;
	while (i < pos && labels[i])
		i++;

	// invalid value?
	if (labels[i] == NULL)
		return OPDI_POSITION_INVALID;

	// join payload
	parts[0] = selectPortLabel;
	parts[1] = port->id;
	parts[2] = position;
	parts[3] = labels[i];
	parts[4] = NULL;

	return send_parts(channel);
}

static uint8_t get_select_port_state(opdi_Port *port) {
	uint8_t result;
	uint16_t pos;
	char position[10];

	if (strcmp(port->type, OPDI_PORTTYPE_SELECT)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	result = opdi_get_select_port_state(port, &pos);
	if (result != OPDI_STATUS_OK)
		return result;

	opdi_uint16_to_str(pos, position);

	// join payload
	parts[0] = selectPortState;
	parts[1] = port->id;
	parts[2] = position;
	parts[3] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

static uint8_t send_select_port_state(channel_t channel, opdi_Port *port) {
	uint8_t result = get_select_port_state(port);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_payload(channel);
}

static uint8_t set_select_port_position(channel_t channel, opdi_Port *port, const char *position) {
	uint8_t result;
	uint16_t pos;
	uint16_t i;
	char **labels;

	if (strcmp(port->type, OPDI_PORTTYPE_SELECT)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	// port info is an array of char*
	labels = (char**)port->info.ptr;

	result = opdi_str_to_uint16(position, &pos);
	if (result != OPDI_STATUS_OK)
		return result;

	// count labels up to pos
	i = 0;
	while (i < pos && labels[i])
		i++;

	// invalid value?
	if (labels[i] == NULL)
		return OPDI_POSITION_INVALID;

	result = opdi_set_select_port_position(port, i);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_select_port_state(channel, port);
}
#endif

/// dial port functions

#ifndef OPDI_NO_DIAL_PORTS
static uint8_t get_dial_port_state(opdi_Port *port) {
	uint8_t result;
	int32_t pos;
	char position[10];

	if (strcmp(port->type, OPDI_PORTTYPE_DIAL)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	result = opdi_get_dial_port_state(port, &pos);
	if (result != OPDI_STATUS_OK)
		return result;

	opdi_int32_to_str(pos, position);

	// join payload
	parts[0] = dialPortState;
	parts[1] = port->id;
	parts[2] = position;
	parts[3] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

static uint8_t send_dial_port_state(channel_t channel, opdi_Port *port) {
	uint8_t result = get_dial_port_state(port);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_payload(channel);
}

static uint8_t set_dial_port_position(channel_t channel, opdi_Port *port, const char *position) {
	uint8_t result;
	int32_t pos;
	opdi_DialPortInfo *dpi;
	uint16_t i;

	if (strcmp(port->type, OPDI_PORTTYPE_DIAL)) {
		return OPDI_WRONG_PORT_TYPE;
	}

	result = opdi_str_to_int32(position, &pos);
	if (result != OPDI_STATUS_OK)
		return result;

	// validate position
	dpi = (opdi_DialPortInfo *)port->info.ptr;

	// invalid value?
	i = pos - dpi->min;
	if ((pos < dpi->min) || (pos > dpi->max) || (i % dpi->step != 0))
		return OPDI_POSITION_INVALID;

	result = opdi_set_dial_port_position(port, pos);
	if (result != OPDI_STATUS_OK)
		return result;

	return send_dial_port_state(channel, port);
}
#endif

/// streaming port functions
#if (OPDI_STREAMING_PORTS > 0)
static uint8_t bind_streaming_port(channel_t channel, opdi_Port *port, const char *bChan) {
	uint8_t result;
	channel_t bChannel;

	// convert channel string to number
#if (channel_bits == 8)
	result = opdi_str_to_uint8(bChan, &bChannel);
#elif (channel_bits == 16)
	result = opdi_str_to_uint16(bChan, &bChannel);
#else
#error "Not implemented; unable to convert channel string to numeric value"
#endif

	if (result != OPDI_STATUS_OK)
		return result;

	// channel sanity check
	if (bChannel <= 0)
		return OPDI_CHANNEL_INVALID;

	// bind port
	result = opdi_bind_port(port, bChannel);

	// problem
	if (result == OPDI_TOO_MANY_BINDINGS) {
		send_disagreement(channel, result, NULL, NULL);
		return OPDI_STATUS_OK;
	}

	// error
	if (result != OPDI_STATUS_OK)
		return result;

	// ok
	return send_agreement(channel);
}

static uint8_t unbind_streaming_port(channel_t channel, opdi_Port *port) {
	uint8_t result;

	// unbind port
	result = opdi_unbind_port(port);

	// error
	if (result != OPDI_STATUS_OK)
		return result;

	// ok
	return send_agreement(channel);
}
#endif

#ifdef EXTENDED_PROTOCOL
// send all port states in one message
static uint8_t send_all_port_states(channel_t channel) {
	opdi_Message message;
	opdi_Port *port;
	uint8_t result;
	char buffer[OPDI_MESSAGE_PAYLOAD_LENGTH];
	char *current = buffer;
	uint16_t remaining_length = OPDI_MESSAGE_PAYLOAD_LENGTH;
	uint16_t cur_length = 0;
	uint16_t length;
	buffer[cur_length] = '\0';

	// go through list of device ports
	port = opdi_get_ports();
	while (port != NULL) {
		payload[0] = '\0';

		// write port state to payload global variable
#ifndef OPDI_NO_DIGITAL_PORTS
		if (strcmp(port->type, OPDI_PORTTYPE_DIGITAL) == 0) {
			result = get_digital_port_state(port);
			if (result != OPDI_STATUS_OK)
				return result;
		}
		else
#endif
#ifndef OPDI_NO_ANALOG_PORTS
		if (strcmp(port->type, OPDI_PORTTYPE_ANALOG) == 0) {
			result = get_analog_port_state(port);
			if (result != OPDI_STATUS_OK)
				return result;
		}
		else
#endif
#ifndef OPDI_NO_SELECT_PORTS
		if (strcmp(port->type, OPDI_PORTTYPE_SELECT) == 0) {
			result = get_select_port_state(port);
			if (result != OPDI_STATUS_OK)
				return result;
		}
		else
#endif
#ifndef OPDI_NO_DIAL_PORTS
		if (strcmp(port->type, OPDI_PORTTYPE_DIAL) == 0) {
			result = get_dial_port_state(port);
			if (result != OPDI_STATUS_OK)
				return result;
		}
#endif
		// check payload length
		length = strlen(payload);
		if (length > remaining_length)
			return OPDI_ERROR_MSGBUF_OVERFLOW;
		// copy payload to current buffer
		strcpy(current, payload);

		port = port->next;

		if ((port != NULL) && (length > 0)) {
			current[length] = MULTIMESSAGE_SEPARATOR;
			length++;
			current += length;
			remaining_length -= length;
		}
	}

	// send on the same channel
	message.channel = channel;
	message.payload = buffer;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;
	
	return OPDI_STATUS_OK;
}
#endif		// EXTENDED_PROTOCOL

/** Implements the basic protocol message handler.
*/
static uint8_t basic_protocol_message(channel_t channel) {
	uint8_t result;
	opdi_Port *port;

	// we can be sure to have no control channel messages here
	// so we don't have to handle Disconnect etc.

	if (!strcmp(parts[0], getDeviceCaps)) {
		// get device capabilities
		send_device_caps(channel);
	} 
	else if (!strcmp(parts[0], getPortInfo)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		result = send_port_info(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
#ifndef OPDI_NO_ANALOG_PORTS
	else if (!strcmp(parts[0], getAnalogPortState)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		result = send_analog_port_state(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setAnalogPortValue)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// set the value
		result = set_analog_port_value(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setAnalogPortMode)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// set the value
		result = set_analog_port_mode(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setAnalogPortResolution)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// set the value
		result = set_analog_port_resolution(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setAnalogPortReference)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// set the value
		result = set_analog_port_reference(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
#endif
#ifndef OPDI_NO_DIGITAL_PORTS
	else if (!strcmp(parts[0], getDigitalPortState)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		result = send_digital_port_state(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setDigitalPortLine)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		result = set_digital_port_line(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setDigitalPortMode)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;

		result = set_digital_port_mode(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	}
#endif
#ifndef OPDI_NO_SELECT_PORTS
	else if (!strcmp(parts[0], getSelectPortLabel)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		result = send_select_port_label(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], getSelectPortState)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		result = send_select_port_state(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setSelectPortPosition)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		result = set_select_port_position(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
#endif
#ifndef OPDI_NO_DIAL_PORTS
	else if (!strcmp(parts[0], getDialPortState)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		result = send_dial_port_state(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else if (!strcmp(parts[0], setDialPortPosition)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		result = set_dial_port_position(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
#endif
#if (OPDI_STREAMING_PORTS > 0)
	else if (!strcmp(parts[0], bindStreamingPort)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		if (parts[2] == NULL)
			return OPDI_PROTOCOL_ERROR;
		result = bind_streaming_port(channel, port, parts[2]);
		if (result != OPDI_STATUS_OK)
			return result;
	}
	else if (!strcmp(parts[0], unbindStreamingPort)) {
		if (parts[1] == NULL)
			return OPDI_PROTOCOL_ERROR;
		// find port
		port = opdi_find_port_by_id(parts[1]);
		if (port == NULL)
			return OPDI_PORT_UNKNOWN;
		result = unbind_streaming_port(channel, port);
		if (result != OPDI_STATUS_OK)
			return result;
	}
#endif
	else {
		// unknown message received
		// ignore for now
		return OPDI_STATUS_OK;
	}
	
#ifdef OPDI_HAS_MESSAGE_HANDLED
	// notify the device that a message has been handled
	return opdi_message_handled(channel, parts);
#else
	return OPDI_STATUS_OK;
#endif
}

#ifdef EXTENDED_PROTOCOL
/** Implements the basic protocol message handler.
*/
static uint8_t extended_protocol_message(channel_t channel) {
	uint8_t result;
	
	// only handle messages of the extended protocol here
	if (!strcmp(parts[0], getAllPortStates)) {
		result = send_all_port_states(channel);
		if (result != OPDI_STATUS_OK)
			return result;
	} 
	else
		// for all other messages, fall back to the basic protocol
		return basic_protocol_message(channel);
		
#ifdef OPDI_HAS_MESSAGE_HANDLED
	// notify the device that a message has been handled
	return opdi_message_handled(channel, parts);
#else
	return OPDI_STATUS_OK;
#endif
}
#endif

/** The message handler for the basic protocol.
*/
uint8_t opdi_handle_basic_message(opdi_Message *m) {
	uint8_t result;

#if (OPDI_STREAMING_PORTS > 0)
	// try to dispatch message to streaming ports
	result = opdi_try_dispatch_stream(m);
	if (result != OPDI_NO_BINDING)
		return result;

	// there is no binding for the message's channel
#endif

	result = strings_split(m->payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, NULL);
	if (result != OPDI_STATUS_OK)
		return result;

	// let the protocol handle the message
	result = basic_protocol_message(m->channel);
	if (result != OPDI_STATUS_OK) {
		// special case: slave function wants to disconnect
		if (result == OPDI_DISCONNECTED)
			return opdi_disconnect();
		// error case
		return send_error(result, NULL, NULL);
	}

	return OPDI_STATUS_OK;
}

/** The protocol handler for the basic protocol.
*/
static uint8_t basic_protocol_handler(void) {
	opdi_Message m;
	uint8_t result;

	// enter processing loop
	while (1) {
		// because this call is blocking, it will result
		// in a timeout error if no ping message is coming in any more
		// causing the termination of the connection
		result = opdi_get_message(&m, OPDI_CAN_SEND);
		if (result != OPDI_STATUS_OK)
			return result;

		result = strings_split(m.payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, NULL);
		if (result != OPDI_STATUS_OK)
			return result;

		// message on control channel?
		if (m.channel == 0) {
			// disconnect message?
			if (!strcmp(parts[0], disconnect))
				return OPDI_DISCONNECTED;

			// error message?
			if (!strcmp(parts[0], error))
				return OPDI_DEVICE_ERROR;

			// debug message?
			if (!strcmp(parts[0], debug)) {
				result = opdi_debug_msg((uint8_t *)parts[1], OPDI_DIR_DEBUG);
				if (result != OPDI_STATUS_OK)
					return result;
			}
			
#ifdef OPDI_HAS_MESSAGE_HANDLED
			// notify the device that a message has been handled
			result = opdi_message_handled(0, parts);
			if (result != OPDI_STATUS_OK) {
				// intentional disconnects are not an error
				if (result != OPDI_DISCONNECTED)
					// an error occurred during message handling; send error to device and exit
					return send_error(result, NULL, NULL);
				return result;
			}
#endif
		} else {
			// message other than control message received
			// let the protocol handle the message
			result = basic_protocol_message(m.channel);
			if (result != OPDI_STATUS_OK) {
				// intentional disconnects are not an error
				if (result != OPDI_DISCONNECTED)
					// an error occurred during message handling; send error to device and exit
					return send_error(result, NULL, NULL);
				return result;
			}
		}
	}	// while
}

#ifdef EXTENDED_PROTOCOL
/** The protocol handler for the extended protocol.
*/
static uint8_t extended_protocol_handler(void) {
	opdi_Message m;
	uint8_t result;

	// enter processing loop
	while (1) {
		// because this call is blocking, it will automatically
		// result in an error if no ping message is coming in any more
		result = opdi_get_message(&m, OPDI_CAN_SEND);
		if (result != OPDI_STATUS_OK)
			return result;

		result = strings_split(m.payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, NULL);
		if (result != OPDI_STATUS_OK)
			return result;

		// message on control channel?
		if (m.channel == 0) {
			// disconnect message?
			if (!strcmp(parts[0], disconnect))
				return OPDI_DISCONNECTED;

			// error message?
			if (!strcmp(parts[0], error))
				return OPDI_DEVICE_ERROR;

			// debug message?
			if (!strcmp(parts[0], debug))
				opdi_debug_msg((uint8_t *)parts[1], OPDI_DIR_DEBUG);
		} else {
			// message other than control message received
			// let the protocol handle the message
			result = extended_protocol_message(m.channel);
			if (result != OPDI_STATUS_OK) {
				// intentional disconnects are not an error
				if (result != OPDI_DISCONNECTED)
					// an error occurred during message handling; send error to device and exit
					return send_error(result, NULL, NULL);
				return result;
			}
		}
	}	// while
}
#endif

/** Prepares the OPDI protocol implementation for a new connection.
*   Most importantly, this will disable encryption as it is not used at the beginning
*   of the handshake.
*/
extern uint8_t opdi_init() {
#ifndef OPDI_NO_ENCRYPTION
	// handshake starts unencrypted
	opdi_set_encryption(OPDI_DONT_USE_ENCRYPTION);
#endif

	return OPDI_STATUS_OK;
}

/* Performs the handshake and runs the message processing loop if successful.
*  Errors during the handshake are not sent to the connected device.
*/
uint8_t opdi_start(opdi_Message *message, opdi_GetProtocol get_protocol, opdi_ProtocolCallback protocol_callback) {
#define MAX_ENCRYPTIONS		3
	opdi_Message m;
	uint8_t result;
	uint8_t partCount;
	int32_t flags;
	char buf[10];
	opdi_ProtocolHandler protocol_handler = &basic_protocol_handler;
	
#ifndef OPDI_NO_ENCRYPTION
	const char *encryptions[MAX_ENCRYPTIONS];
	uint8_t i;
	const char *encryption = "";
	uint8_t use_encryption = 0;
#endif
#ifndef OPDI_NO_AUTHENTICATION
	uint32_t savedTimeout;
#endif

#if (OPDI_STREAMING_PORTS > 0)
	// initiate a new connection: clear port bindings
	opdi_reset_bindings();
#endif

	if (protocol_callback != NULL)
		protocol_callback(OPDI_PROTOCOL_START_HANDSHAKE);

	////////////////////////////////////////////////////////////
	///// Received: Handshake
	////////////////////////////////////////////////////////////

	// expect a message	on the control channel
	if (message->channel != 0)
		return OPDI_PROTOCOL_ERROR;

	result = strings_split(message->payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, &partCount);
	if (result != OPDI_STATUS_OK)
		return result;

	if (partCount != 4)
		return OPDI_PROTOCOL_ERROR;

	// handshake tag must match
	if (strcmp(parts[0], handshake))
		return OPDI_PROTOCOL_ERROR;

	// protocol version must match
	if (strcmp(parts[1], handshake_version))
		return OPDI_PROTOCOL_ERROR;

	// convert flags
	result = opdi_str_to_int32(parts[2], &flags);
	if (result != OPDI_STATUS_OK)
		return result;

#ifdef OPDI_NO_ENCRYPTION
	// is encryption required by the master?
	if (flags & OPDI_FLAG_ENCRYPTION_REQUIRED)
		// this device does not support encryption
		return send_disagreement(OPDI_ENCRYPTION_NOT_SUPPORTED, 0, NULL, NULL);
#else
	// encryption is supported
	// split supported encryptions
	result = strings_split(parts[3], ',', encryptions, MAX_ENCRYPTIONS, 1, &partCount);
	if (result != OPDI_STATUS_OK)
		return result;

	// is encryption required by the master?
	if ((flags & OPDI_FLAG_ENCRYPTION_REQUIRED) == OPDI_FLAG_ENCRYPTION_REQUIRED) {
		// does the device not allow or support encryption?
		if (((opdi_device_flags & OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) == OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) || (strlen(opdi_encryption_method) == 0))
			return send_disagreement(0, OPDI_ENCRYPTION_NOT_SUPPORTED, "Encryption not supported: ", "by device");

		// device's encryption must be supported
		result = OPDI_ENCRYPTION_NOT_SUPPORTED;
		for (i = 0; i < partCount; i++) {
			if (!strcmp(encryptions[i], opdi_encryption_method)) {
				result = OPDI_STATUS_OK;
				break;
			}
		}
		if (result != OPDI_STATUS_OK)
			return send_disagreement(0, result, "Encryption not supported: ", opdi_encryption_method);
		// choose encryption
		encryption = opdi_encryption_method;
		use_encryption = 1;
	} 
	// does the device require encryption?
	else if ((opdi_device_flags & OPDI_FLAG_ENCRYPTION_REQUIRED) == OPDI_FLAG_ENCRYPTION_REQUIRED) {
		// does the master not allow encryption?
		if (flags & OPDI_FLAG_ENCRYPTION_NOT_ALLOWED)
			return send_disagreement(0, OPDI_ENCRYPTION_REQUIRED, "Encryption required: ", "by device");

		// device's encryption must be supported
		result = OPDI_ENCRYPTION_REQUIRED;
		for (i = 0; i < partCount; i++) {
			if (!strcmp(encryptions[i], opdi_encryption_method)) {
				result = OPDI_STATUS_OK;
				break;
			}
		}
		if (result != OPDI_STATUS_OK)
			return send_disagreement(0, result, "Encryption required: ", opdi_encryption_method);
		// choose encryption
		encryption = opdi_encryption_method;
		use_encryption = 1;
	}
	else {
		// encryption is optional

		// not forbidden by device flags?
		if ((opdi_device_flags & OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) != OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) {
			// device's encryption may be supported
			for (i = 0; i < partCount; i++) {
				if (!strcmp(encryptions[i], opdi_encryption_method)) {
					// choose encryption
					encryption = opdi_encryption_method;
					use_encryption = 1;
					break;
				}
			}
		}
	}
#endif	// OPDI_NO_ENCRYPTION

	////////////////////////////////////////////////////////////
	///// Send: Handshake reply
	////////////////////////////////////////////////////////////

	// prepare handshake reply message
	m.channel = 0;
	m.payload = payload;
	parts[0] = handshake;
	parts[1] = handshake_version;
	parts[2] = (opdi_encoding != NULL ? opdi_encoding : "");
#ifndef OPDI_NO_ENCRYPTION
	parts[3] = encryption;
#else
	parts[3] = "";
#endif
	// convert flags to string
	opdi_int32_to_str(opdi_device_flags, buf);
	parts[4] = buf;
	parts[5] = opdi_supported_protocols;
	parts[6] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	result = opdi_put_message(&m);
	if (result != OPDI_STATUS_OK)
		return result;

#ifndef OPDI_NO_ENCRYPTION
	// if encryption is used, switch it on
	if (use_encryption) {
		opdi_set_encryption(OPDI_USE_ENCRYPTION);
	}
#endif

	////////////////////////////////////////////////////////////
	///// Receive: Protocol Select
	////////////////////////////////////////////////////////////

	result = expect_control_message(parts, &partCount);
	if (result != OPDI_STATUS_OK)
		return result;

	if (partCount != 3)
		return OPDI_PROTOCOL_ERROR;
		
#ifdef EXTENDED_PROTOCOL
	// check extended protocol implementation
	if (strcmp(parts[0], extended_protocol_magic) == 0) {
		protocol_handler = &extended_protocol_handler;
	}
#endif
	else
	// check chosen protocol implementation
	if (strcmp(parts[0], basic_protocol_magic)) {
		// not the basic protocol, use device supplied function to determine protocol handler
		if (get_protocol == NULL)
			return OPDI_PROTOCOL_NOT_SUPPORTED;
		protocol_handler = get_protocol(parts[0]);
		// protocol not registered
		if (protocol_handler == NULL)
			// fallback to basic
			protocol_handler = &basic_protocol_handler;
	}

	// copy master's name
	strncpy(opdi_master_name, (char*)parts[2], OPDI_MASTER_NAME_LENGTH - 1);

	// pass preferred languages, see opdi_device.h
	result = opdi_choose_language(parts[1]);
	if (result != OPDI_STATUS_OK)
		return result;

	////////////////////////////////////////////////////////////
	///// Send: Slave Name
	////////////////////////////////////////////////////////////
		
	m.channel = 0;
	m.payload = payload;
	parts[0] = agreement;
	parts[1] = opdi_slave_name;
	parts[2] = NULL;

	result = strings_join(parts, PARTS_SEPARATOR, payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	result = opdi_put_message(&m);
	if (result != OPDI_STATUS_OK)
		return result;

#ifdef OPDI_NO_AUTHENTICATION
	// is authentication required by the master?
	if (flags & OPDI_FLAG_AUTHENTICATION_REQUIRED)
		// this device does not support authentication
		return send_disagreement(OPDI_AUTH_NOT_SUPPORTED, 0, NULL, NULL);
#else
	// does this device require authentication?
	if (opdi_device_flags & OPDI_FLAG_AUTHENTICATION_REQUIRED) {

		////////////////////////////////////////////////////////////
		///// Receive: Authentication
		////////////////////////////////////////////////////////////

		// increase the timeout for this procedure (the user may have to enter credentials first)
		savedTimeout = opdi_get_timeout();
		opdi_set_timeout(OPDI_AUTHENTICATION_TIMEOUT);
		result = opdi_get_message(&m, OPDI_CANNOT_SEND);
		// set the saved timeout back
		opdi_set_timeout(savedTimeout);
		if (result != OPDI_STATUS_OK)
			return result;

		result = strings_split(m.payload, PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 0, NULL);	// no trim!
		if (result != OPDI_STATUS_OK)
			return result;

		if (strcmp(parts[0], auth))
			return send_disagreement(0, OPDI_AUTHENTICATION_EXPECTED, NULL, NULL);

		// user name: match case insensitive
		if (opdi_string_cmp(parts[1], opdi_username) || strcmp(parts[2], opdi_password)) {
			return send_disagreement(0, OPDI_AUTHENTICATION_FAILED, "Authentication failed", NULL);
		}

		result = send_agreement(0);
		if (result != OPDI_STATUS_OK)
			return result;
	}
#endif	// OPDI_NO_AUTHENTICATION

	if (protocol_callback != NULL)
		protocol_callback(OPDI_PROTOCOL_CONNECTED);

	// start the protocol
	result = protocol_handler();

	if (protocol_callback != NULL)
		protocol_callback(OPDI_PROTOCOL_DISCONNECTED);

	return result;
}


/** Sends a debug message to the master.
*/
uint8_t opdi_send_debug(char *debugmsg) {
	parts[0] = debug;
	parts[1] = debugmsg;
	parts[2] = NULL;

	// send the parts on the control channel
	return send_parts(0);
}

/** Causes the Reconfigure message to be sent which prompts the master to re-read the device capabilities.
*/
uint8_t opdi_reconfigure(void) {
	// send a reconfigure message on the control channel
	opdi_Message message;
	uint8_t result;

	// send on the control channel
	message.channel = 0;
	message.payload = (char *)reconfigure;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

/** Causes the Refresh message to be sent for the specified ports. The last element must be NULL.
*   If the first element is NULL, sends the empty refresh message causing all ports to be
*   refreshed.
*/
uint8_t opdi_refresh(opdi_Port **ports) {
	opdi_Port *port = ports[0];
	uint8_t i = 1;

	// prepare the parts
	parts[0] = refresh;
	// iterate over all specified ports
	while (port != NULL) {
		parts[i] = port->id;
		port = ports[i++];
		if (i >= OPDI_MAX_MESSAGE_PARTS)
			return OPDI_ERROR_PARTS_OVERFLOW;
	}
	parts[i] = NULL;

	// send the parts
	return send_parts(0);
}

/** Causes the Disconnect message to be sent to the master.
*   Returns OPDI_DISCONNECTED. After this, no more messages may be sent to the master.
*/
uint8_t opdi_disconnect(void) {
	// send a disconnect message on the control channel
	opdi_Message message;
	uint8_t result;

	// send on the control channel
	message.channel = 0;
	message.payload = (char *)disconnect;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_DISCONNECTED;
}

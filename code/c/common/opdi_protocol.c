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

// Common functions of the OPDI protocol.

#include <stdlib.h>
#include <string.h>

#include "opdi_constants.h"
#include "opdi_strings.h"
#include "opdi_message.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_protocol_constants.h"
#include "opdi_config.h"
#include "opdi_platformfuncs.h"
#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"

// for splitting messages into parts
const char *opdi_msg_parts[OPDI_MAX_MESSAGE_PARTS];
// for assembling a payload
char opdi_msg_payload[OPDI_MESSAGE_PAYLOAD_LENGTH];

// expects a control message on channel 0
uint8_t expect_control_message(const char **parts, uint8_t *partCount) {
	opdi_Message m;
	uint8_t result;

	result = opdi_get_message(&m, OPDI_CANNOT_SEND);
	if (result != OPDI_STATUS_OK)
		return result;

	// message must be on control channel
	if (m.channel != 0)
		return OPDI_PROTOCOL_ERROR;

	result = strings_split(m.payload, OPDI_PARTS_SEPARATOR, parts, OPDI_MAX_MESSAGE_PARTS, 1, partCount);
	if (result != OPDI_STATUS_OK)
		return result;

	// disconnect message?
	if (!strcmp(parts[0], OPDI_Disconnect))
		return OPDI_DISCONNECTED;

	// error message?
	if (!strcmp(parts[0], OPDI_Error))
		return OPDI_DEVICE_ERROR;

	return OPDI_STATUS_OK;
}

uint8_t send_error(uint8_t code, const char *part1, const char *part2) {
	// send an error message on the control channel
	char buf[BUFSIZE_8BIT];
	opdi_Message message;
	uint8_t result;

	opdi_uint8_to_str(code, buf);

	// join payload
	opdi_msg_parts[0] = OPDI_Error;
	opdi_msg_parts[1] = buf;
	opdi_msg_parts[2] = part1;
	opdi_msg_parts[3] = part2;
	opdi_msg_parts[4] = NULL;

	result = strings_join(opdi_msg_parts, OPDI_PARTS_SEPARATOR, opdi_msg_payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	// send on the control channel
	message.channel = 0;
	message.payload = opdi_msg_payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return code;
}

uint8_t send_disagreement(channel_t channel, uint8_t code, const char *part1, const char *part2) {
	// send a disagreement message on the specified channel
	char buf[BUFSIZE_8BIT];
	opdi_Message message;
	uint8_t result;

	opdi_uint8_to_str(code, buf);

	// join payload
	opdi_msg_parts[0] = OPDI_Disagreement;
	opdi_msg_parts[1] = buf;
	opdi_msg_parts[2] = part1;
	opdi_msg_parts[3] = part2;
	opdi_msg_parts[4] = NULL;

	result = strings_join(opdi_msg_parts, OPDI_PARTS_SEPARATOR, opdi_msg_payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	message.channel = channel;
	message.payload = opdi_msg_payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

uint8_t send_port_error(channel_t channel, const char *portID, const char *part1, const char *part2) {
	// send a port error message on the specified channel
	opdi_Message message;
	uint8_t result;

	// join payload
	opdi_msg_parts[0] = OPDI_Error;
	opdi_msg_parts[1] = portID;
	opdi_msg_parts[2] = part1;
	opdi_msg_parts[3] = part2;
	opdi_msg_parts[4] = NULL;

	result = strings_join(opdi_msg_parts, OPDI_PARTS_SEPARATOR, opdi_msg_payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	message.channel = channel;
	message.payload = opdi_msg_payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

#if (OPDI_STREAMING_PORTS > 0) || !defined(OPDI_NO_AUTHENTICATION)
uint8_t send_agreement(channel_t channel) {
	// send an agreement message on the specified channel
	opdi_Message message;
	uint8_t result;

	// join payload
	opdi_msg_parts[0] = OPDI_Agreement;
	opdi_msg_parts[1] = NULL;

	result = strings_join(opdi_msg_parts, OPDI_PARTS_SEPARATOR, opdi_msg_payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	message.channel = channel;
	message.payload = opdi_msg_payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}
#endif

/** Common function: send the contents of the opdi_msg_parts array on the specified channel.
*/
uint8_t send_payload(channel_t channel) {
	opdi_Message message;
	uint8_t result;

	message.channel = channel;
	message.payload = opdi_msg_payload;

	result = opdi_put_message(&message);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

/** Common function: send the contents of the opdi_msg_parts array on the specified channel.
*/
uint8_t send_parts(channel_t channel) {
	uint8_t result;

	result = strings_join(opdi_msg_parts, OPDI_PARTS_SEPARATOR, opdi_msg_payload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (result != OPDI_STATUS_OK)
		return result;

	result = send_payload(channel);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

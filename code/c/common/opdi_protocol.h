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
    
#ifndef __OPDI_PROTOCOL_H
#define __OPDI_PROTOCOL_H

#include "opdi_platformtypes.h"

// Common functions of the OPDI protocol.

// for splitting messages into parts
extern const char *opdi_msg_parts[];
// for assembling a payload
extern char opdi_msg_payload[];

// expects a control message on channel 0
extern uint8_t expect_control_message(const char **parts, uint8_t *partCount);

// sends an error with optional additional information
extern uint8_t send_error(uint8_t code, const char *part1, const char *part2);

// sends an isagreement message with optional additional information
extern uint8_t send_disagreement(channel_t channel, uint8_t code, const char *part1, const char *part2);

// sends an agreement message
extern uint8_t send_agreement(channel_t channel);

// sends the contents of the opdi_msg_parts array on the specified channel
extern uint8_t send_payload(channel_t channel);

// sends the contents of the opdi_msg_parts array on the specified channel
extern uint8_t send_parts(channel_t channel);

#endif		// __OPDI_PROTOCOL_H

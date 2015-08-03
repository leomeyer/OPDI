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
   
// Implements the messaging layer. Synchronous sending and receiving of messages.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_config.h"
#include "opdi_message.h"
#include "opdi_constants.h"

#define MESSAGE_TERMINATOR	'\n'
#define MESSAGE_SEPARATOR	':'
#define CHANNEL_MAXBUF	3				// maximum size of channel digits

#define MESSAGE_MALFORMED	"malformed msg:"
#define MESSAGE_UNKNOWN		"unknown msg:"

// the message payload buffer
static char msgPayload[OPDI_MESSAGE_PAYLOAD_LENGTH];

// the message output buffer
static uint8_t msgBuf[OPDI_MESSAGE_BUFFER_SIZE];

// function handler for receiving of bytes
static func_receive receive;

// function handler for sending of bytes
static func_send send;

// info for receive and send functions
static void* sendinfo;

// the timeout used for receiving messages (in milliseconds)
static uint16_t message_timeout = OPDI_DEFAULT_MESSAGE_TIMEOUT;

#ifndef OPDI_NO_ENCRYPTION

// flag whether encryption is on or off
static uint8_t encryption;

#endif

/** Compares cs with the four byte hexadecimal checksum value starting at bytes[pos] and returns 0 if ok.
*/
static uint16_t compare_checksum(uint16_t cs, uint8_t bytes[], uint16_t pos) {
	uint16_t value = 0;
	uint8_t byte;
	uint16_t i;
	for (i = 0; i < 4; i++) {
		byte = bytes[pos + 3 - i];
		if ((byte >= '0') && (byte <= '9')) value |= (byte - '0') << i * 4;
		else if ((byte >= 'A') && (byte <= 'F')) value |= (byte - 'A' + 10) << i * 4;
		else if ((byte >= 'a') && (byte <= 'f')) value |= (byte - 'a' + 10) << i * 4;
		else
			return OPDI_ERROR_CONVERSION;
	}
	if (cs != value)
		return OPDI_ERROR_MALFORMED_MESSAGE;
	return OPDI_STATUS_OK;
}

/** Decode the message from bytes using msgPayload as buffer. Returns an error code if it can't be decoded.
*/
static uint8_t decode(opdi_Message *message, uint8_t bytes[]) {
	char channelBuf[CHANNEL_MAXBUF + 1] = {'\0'};
	uint8_t pos = 0;
	uint8_t payloadPos = 0;
	uint8_t lastSepPos = 0;
	uint16_t checksum = 0;
	uint16_t i;
	uint8_t err;

	// detect separator
	while (bytes[pos]) {
		if (bytes[pos] == MESSAGE_SEPARATOR) break;
		channelBuf[pos] = bytes[pos];
		checksum += bytes[pos];
		pos++;
		if (pos >= CHANNEL_MAXBUF)
			// separator not detected within the first few characters
			return OPDI_ERROR_MALFORMED_MESSAGE;
	}

	// parse the channel number
	if (opdi_str_to_uint16(channelBuf, &(message->channel)) != OPDI_STATUS_OK)
		return OPDI_ERROR_MALFORMED_MESSAGE;

	checksum += bytes[pos];
	payloadPos = ++pos;		// start of payload

	// determine end of payload
	while (bytes[pos]) {
		if (bytes[pos] == MESSAGE_SEPARATOR) {
			lastSepPos = pos;
		}
		checksum += bytes[pos];
		pos++;
		if (pos >= OPDI_MESSAGE_BUFFER_SIZE)
			// not properly null-terminated
			return OPDI_ERROR_MALFORMED_MESSAGE;
	}

	if (lastSepPos <= payloadPos)
		// no subsequent separator (checksum missing)
		return OPDI_ERROR_MALFORMED_MESSAGE;

	// subtract the checksum characters
	for (i = pos - 1; i > pos - 5; i--) {
		if (i < 0)
			// message too short
			return OPDI_ERROR_MALFORMED_MESSAGE;
		checksum -= bytes[i];
	}
	if (bytes[i] != MESSAGE_SEPARATOR)
		// checksum separator expected but not found
		return OPDI_ERROR_MALFORMED_MESSAGE;
	checksum -= bytes[i];

	// compare the checksum
	if (compare_checksum(checksum, bytes, i + 1) != OPDI_STATUS_OK)
			// checksum wrong
			return OPDI_ERROR_MALFORMED_MESSAGE;

	// retrieve the payload
	err = opdi_bytes_to_string(bytes, payloadPos, lastSepPos - payloadPos, msgPayload, OPDI_MESSAGE_PAYLOAD_LENGTH);
	if (err != OPDI_STATUS_OK)
		return err;
	message->payload = msgPayload;
	return OPDI_STATUS_OK;
}

/** Encodes the message into msgBuf. Returns an error code if it can't be encoded.
*   Returns the length of the result in length.
*/
static uint8_t encode(opdi_Message *message, uint16_t *length) {
	char channelBuf[CHANNEL_MAXBUF + 1] = {'\0'};
	size_t pos = 0;
	uint16_t checksum = 0;
	size_t i;
	uint8_t err;
	uint16_t bytelen = 0;
	uint8_t nibble;

	// write the channel number
#if (channel_bits == 8)
	pos += opdi_uint8_to_str(message->channel, channelBuf);
#elif (channel_bits == 16)
	pos += opdi_uint16_to_str(message->channel, channelBuf);
#else
#error "Not implemented; unable to convert channel string to numeric value"
#endif

	// transfer channel number
	err = opdi_string_to_bytes(channelBuf, msgBuf, 0, OPDI_MESSAGE_BUFFER_SIZE, &bytelen);
	if (err != OPDI_STATUS_OK)
		return err;

	msgBuf[pos++] = MESSAGE_SEPARATOR;
	if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)
		return OPDI_ERROR_MSGBUF_OVERFLOW;

	// channel checksum
	for (i = pos; i > 0; i--)
		checksum += msgBuf[i - 1];

	// transfer payload
	err = opdi_string_to_bytes(message->payload, msgBuf, pos, OPDI_MESSAGE_BUFFER_SIZE, &bytelen);
	if (err != OPDI_STATUS_OK)
		return err;

	// payload checksum
	i = pos;
	while (pos < bytelen + i) {
		// check: terminator may not occur
		if (msgBuf[pos] == MESSAGE_TERMINATOR)
			return OPDI_TERMINATOR_IN_PAYLOAD;
		checksum += msgBuf[pos++];
		if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)
			return OPDI_ERROR_MSGBUF_OVERFLOW;
	}

	// checksum separator
	msgBuf[pos++] = MESSAGE_SEPARATOR;
	if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)
		return OPDI_ERROR_MSGBUF_OVERFLOW;

	// add the checksum characters
	for (i = 4; i > 0; i--) {
		nibble = (checksum >> (4 * (i - 1))) & 0x0f;
		if (nibble >= 10)
			msgBuf[pos++] = 'a' + nibble - 10;
		else
			msgBuf[pos++] = '0' + nibble;
		if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)
			return OPDI_ERROR_MSGBUF_OVERFLOW;
	}
	if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)
		return OPDI_ERROR_MSGBUF_OVERFLOW;
	msgBuf[pos++] = '\n';

	*length = pos;
	return OPDI_STATUS_OK;
}

uint8_t opdi_message_setup(func_receive recv, func_send snd, void *info) {
#ifndef OPDI_NO_ENCRYPTION
	// if encryption is used, switch it off
	opdi_set_encryption(OPDI_DONT_USE_ENCRYPTION);
#endif
	receive = recv;
	send = snd;
	sendinfo = info;
	return OPDI_STATUS_OK;
}

#ifndef OPDI_NO_ENCRYPTION

static uint8_t get_encrypted(opdi_Message *message, uint8_t can_send) {
#ifdef _MSC_VER
	// compiler can't handle non-constant-length array on the stack
	// use implementation-provided buffer
	uint8_t *buf;
#else
	// compiler can handle non-constant-length array on the stack
	uint8_t buf[opdi_encryption_blocksize];
#endif
	uint16_t pos;
	uint16_t blockpos;
	uint8_t byte;
	uint8_t result;
	uint16_t i;

#ifdef _MSC_VER
	buf = opdi_encryption_buffer;
#endif

	pos = 0;
	blockpos = 0;
	while (1) {
		// read the next byte
		// A receive implementation may send if waiting for a new message
		result = receive(sendinfo, &byte, message_timeout, (can_send && ((pos == 0) && (blockpos == 0)) ? 1 : 0));
		// error or disconnected?
		if (result != OPDI_STATUS_OK)
			return result;

		// place in buffer
		buf[blockpos++] = byte;
		if (pos + blockpos >= OPDI_MESSAGE_BUFFER_SIZE) {
			// ignore overflowing messages
			pos = 0;
			blockpos = 0;
		} else
		// block full?
		if (blockpos >= opdi_encryption_blocksize) {
			// decrypt the block
			result = opdi_decrypt_block(msgBuf + pos,  buf);
			if (result != OPDI_STATUS_OK)
				// encryption error; can't notify the master because it expects an encrypted message which can't be sent
				// this is sort of a dilemma here
				return result;

			pos += opdi_encryption_blocksize;
			// go through decrypted block
			for (i = pos - opdi_encryption_blocksize; i < pos; i++) {
				// is the byte a message terminator?
				if (msgBuf[i] == MESSAGE_TERMINATOR) {
					// the message is finished
					msgBuf[i] = '\0';
					if (decode(message, msgBuf) == OPDI_STATUS_OK) {
						#ifndef OPDI_NO_ENCRYPTION
						if (encryption)
					        	opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING_ENCR);
						else
						#endif
						opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING);
						return OPDI_STATUS_OK;
					}
					// ignore malformed messages
                                        #ifndef OPDI_NO_ENCRYPTION
                                                if (encryption) {
                                        		opdi_debug_msg(MESSAGE_MALFORMED, OPDI_DIR_INCOMING_ENCR);
                                        		opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING_ENCR);
						} else
					#endif
					{
						opdi_debug_msg(MESSAGE_MALFORMED, OPDI_DIR_INCOMING);
						opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING);
					}
					pos = 0;
					blockpos = 0;
					break;
				}
			}
			// start over and next block
			blockpos = 0;
		}
	}
	return OPDI_STATUS_OK;
}

// encrypts the bytes in msgBuf and sends them out
static uint8_t put_encrypted(uint16_t length) {
#ifdef _MSC_VER
	// compiler can't handle non-constant-length array on the stack
	// use implementation-provided buffers
	uint8_t *buf;
	uint8_t *destbuf;
#else
	// compiler can handle non-constant-length array on the stack
	uint8_t buf[opdi_encryption_blocksize];
	uint8_t destbuf[opdi_encryption_blocksize];
#endif
	uint16_t block = 0;
	uint16_t pos;
	uint16_t i;
	uint8_t result;

#ifdef _MSC_VER
	buf = opdi_encryption_buffer;
	destbuf = opdi_encryption_buffer_2;
#endif

	while (length > block * opdi_encryption_blocksize) {
		for (i = 0; i < opdi_encryption_blocksize; i++) {
			// source position
			pos = block * opdi_encryption_blocksize + i;
			if (pos < length)
				buf[i] = msgBuf[pos];
			else
				// pad with random byte which may not be the message terminator
				do {
					buf[i] = (uint8_t)rand();
				} while (buf[i] == MESSAGE_TERMINATOR);
		}
		// encrypt the block
		result = opdi_encrypt_block(destbuf, buf);
		if (result != OPDI_STATUS_OK)
			return result;

//		printf("Sending bytes: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", destbuf[0], destbuf[1], destbuf[2], destbuf[3], destbuf[4], destbuf[5], destbuf[6], destbuf[7], destbuf[8], destbuf[9], destbuf[10], destbuf[11], destbuf[12], destbuf[13], destbuf[14], destbuf[15]);

		// send the block
		result = send(sendinfo, destbuf, opdi_encryption_blocksize);
		if (result != OPDI_STATUS_OK)
			return result;
		// next block
		block++;
	}

	return OPDI_STATUS_OK;
}

#endif

uint8_t opdi_get_message(opdi_Message *message, uint8_t can_send) {
	uint8_t msgBuf[OPDI_MESSAGE_BUFFER_SIZE];
	uint16_t pos = 0;
	uint8_t result;
	uint8_t byte;

#ifndef OPDI_NO_ENCRYPTION
	// if encryption is on, use it
	if (encryption)
		return get_encrypted(message, can_send);
#endif

	while (1) {
		// A receive implementation may send if waiting for a new message (pos == 0)
		result = receive(sendinfo, &byte, message_timeout, (can_send && (pos == 0) ? 1 : 0));
		// error or disconnected?
		if (result != OPDI_STATUS_OK) return result;

		// is the byte a message terminator?
		if (byte == MESSAGE_TERMINATOR) {
			// the message is finished
			msgBuf[pos] = '\0';
			if (decode(message, msgBuf) == OPDI_STATUS_OK) {
				opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING);
				return OPDI_STATUS_OK;
			}
			// ignore malformed messages
			opdi_debug_msg(MESSAGE_MALFORMED, OPDI_DIR_INCOMING);
			opdi_debug_msg((const char *)msgBuf, OPDI_DIR_INCOMING);
			pos = 0;
		} else {
			msgBuf[pos] = byte;
			pos++;
			if (pos >= OPDI_MESSAGE_BUFFER_SIZE - 1)		// \0 should fit, too
				// ignore overflowing messages
				pos = 0;
		}
	}
	return OPDI_STATUS_OK;
}

uint8_t opdi_put_message(opdi_Message *message) {
	uint8_t result;
	uint16_t length = 0;

	result = encode(message, &length);
	if (result != OPDI_STATUS_OK)
		return result;

	// for debug output, do not use terminating \n
	msgBuf[length - 1] = '\0';
#ifndef OPDI_NO_ENCRYPTION
        if (encryption)
                opdi_debug_msg((const char *)msgBuf, OPDI_DIR_OUTGOING_ENCR);
	else
#endif
	opdi_debug_msg((const char *)msgBuf, OPDI_DIR_OUTGOING);
	msgBuf[length - 1] = '\n';

#ifndef OPDI_NO_ENCRYPTION
	// if encryption is on, use it
	if (encryption)
		return put_encrypted(length);
#endif

	result = send(sendinfo, msgBuf, length);
	if (result != OPDI_STATUS_OK)
		return result;

	return OPDI_STATUS_OK;
}

#ifndef OPDI_NO_ENCRYPTION

uint8_t opdi_set_encryption(uint8_t enabled) {
	encryption = enabled;
	return OPDI_STATUS_OK;
}

#endif

void opdi_set_timeout(uint16_t timeout) {
	message_timeout = timeout;
}

uint16_t opdi_get_timeout(void) {
	return message_timeout;
}

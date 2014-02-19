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

// Implements device specific attributes and functions for the Linux OPDI slave implementation ("LinOPDI").

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <errno.h>

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_constants.h"
#include "opdi_ports.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "../test/test.h"

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_SERIAL 2

static int connection_mode = 0;
static char first_com_byte = 0;

static unsigned long idle_timeout_ms = 20000;
static unsigned long last_activity = 0;

/** Helper function: Returns current time in milliseconds
*/
static unsigned long GetTickCount()
{
  struct timeval tv;
  if( gettimeofday(&tv, NULL) != 0 )
    return 0;

  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/** For TCP connections, receives a byte from the socket specified in info and places the result in byte.
*   For serial connections, reads a byte from the file handle specified in info and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {
	char c;
	int result;
	opdi_Message m;
	char bmp085[32];
	long ticks = GetTickCount();
	long sendTicks = ticks;

	while (1) {
		// send a message every few ms if canSend
		// independent of connection mode
		if (GetTickCount() - sendTicks >= 830) {
			if (canSend) {
				sendTicks = GetTickCount();

				handle_streaming_ports();
			}
		}

		if (connection_mode == MODE_TCP) {

			int newsockfd = (long)info;

			// try to read data
			result = read(newsockfd, &c, 1);
			if (result < 0) {
				// timed out?
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// possible timeout
					// "real" timeout condition
					if (GetTickCount() - ticks >= timeout)
						return OPDI_TIMEOUT;
				}
				else {
					// other error condition
					perror("ERROR reading from socket");
					return OPDI_NETWORK_ERROR;
				}
			}
			else
			// connection closed?
			if (result == 0)
				// dirty disconnect
				return OPDI_NETWORK_ERROR;
			else
				// a byte has been received
//				printf("%i", c);
				break;
		}
		else
		if (connection_mode == MODE_SERIAL) {
			int fd = (long)info;
			char inputData;
			int bytesRead;
			int err;

			// first byte of connection remembered?
			if (first_com_byte != 0) {
				c = first_com_byte;
				first_com_byte = 0;
				break;
			}

			if ((bytesRead = read(fd, &inputData, 1)) >= 0) {
				if (bytesRead == 1) {
					// a byte has been received
					c = inputData;
					break;
				}
				else {
					// ran into timeout
					// "real" timeout condition
					if (GetTickCount() - ticks >= timeout)
						return OPDI_TIMEOUT;
				}
			}
			else {
				// device error
				return OPDI_DEVICE_ERROR;
			}
		}
	}

	*byte = (uint8_t)c;

	return OPDI_STATUS_OK;
}

/** For TCP connections, sends count bytes to the socket specified in info.
*   For serial connections, writes count bytes to the file handle specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	char *c = (char *)bytes;

	if (connection_mode == MODE_TCP) {

		int newsockfd = (long)info;

		if (write(newsockfd, c, count) < 0) {
			printf("ERROR writing to socket");
			return OPDI_DEVICE_ERROR;
		}
	}
	else
	if (connection_mode == MODE_SERIAL) {
		int fd = (long)info;

		if (write(fd, c, count) != count) {
			return OPDI_DEVICE_ERROR;
		}
	}

	return OPDI_STATUS_OK;
}

static void my_protocol_callback(uint8_t state) {
	if (state == OPDI_PROTOCOL_START_HANDSHAKE) {
		printf("Handshake started\n");
	} else
	if (state == OPDI_PROTOCOL_CONNECTED) {
		printf("Connected to: %s\n", opdi_master_name);
	} else
	if (state == OPDI_PROTOCOL_DISCONNECTED) {
		printf("Disconnected\n");
	}
}

void init_device() {
	// set slave name
	char nameBuf[1024];
	size_t compNameLen = 1024;

	if (opdi_config_name == NULL) {
		opdi_config_name = (const char*)malloc(OPDI_CONFIG_NAME_LENGTH);
	}

	// requires #undef UNICODE to work properly
	if (0 != gethostname(nameBuf, compNameLen)) {
		sprintf((char *)opdi_config_name, "LinOPDI (%s)", nameBuf);
	}
	else {
		opdi_config_name = "LinOPDI";
	}

	// configure ports
	configure_ports();
}

#ifdef __cplusplus
extern "C" {
#endif 

/** This method handles an incoming TCP connection. It blocks until the connection is closed.
*/
int HandleTCPConnection(int csock) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_TCP;
	init_device();

	struct timeval aTimeout;
	aTimeout.tv_sec = 0;
	aTimeout.tv_usec = 1000;		// one ms timeout

	// set timeouts on socket
	if (setsockopt (csock, SOL_SOCKET, SO_RCVTIMEO, (char *)&aTimeout, sizeof(aTimeout)) < 0) {
		printf("setsockopt failed\n");
		return OPDI_DEVICE_ERROR;
	}
	if (setsockopt (csock, SOL_SOCKET, SO_SNDTIMEO, (char *)&aTimeout, sizeof(aTimeout)) < 0) {
		printf("setsockopt failed\n");
		return OPDI_DEVICE_ERROR;
	}

	// info value is the socket handle
	result = opdi_message_setup(&io_receive, &io_send, (void *)(long)csock);
	if (result != 0) 
		return result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

	// release the socket
	return result;
}

/** This method handles an incoming serial connection. It blocks until the connection is closed.
*/
int HandleSerialConnection(char firstByte, int fd) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_SERIAL;
	first_com_byte = firstByte;
	init_device();

	// info value is the serial port handle
	result = opdi_message_setup(&io_receive, &io_send, (void *)(long)fd);
	if (result != 0) 
		return result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

	return result;
}

#ifdef __cplusplus
}
#endif 


uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	uint8_t result;
	if (idle_timeout_ms > 0) {
		// do not time out if there are bound streaming ports
		if (channel != 0 || opdi_get_port_bind_count() > 0) {
			// reset activity time
			last_activity = GetTickCount();
		} else {
			// control channel message
			if (GetTickCount() - last_activity > idle_timeout_ms) {
				result = opdi_send_debug("Session timeout!");
				if (result != OPDI_STATUS_OK)
					return result;
				return opdi_disconnect();
			} else {
				// send a test debug message
				// opdi_send_debug("Session timeout not yet reached");
			}
		}
	}

	return OPDI_STATUS_OK;
}

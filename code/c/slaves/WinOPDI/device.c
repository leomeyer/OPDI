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

// Implements device specific attributes and functions for the Windows OPDI slave implementation ("WinOPDI").

#include <winsock2.h>
#include <Windows.h>
#include <WinBase.h>
#include <Mmsystem.h>
#include <stdio.h>
#pragma comment(lib, "Winmm.lib")

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"
#include "opdi_constants.h"
#include "opdi_ports.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"

#include "../test/test.h"

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_COM 2

static int connection_mode = 0;
static char first_com_byte = 0;

static uint32_t idle_timeout_ms = 20000;
static uint32_t last_activity = 0;

/** For TCP connections, receives a byte from the socket specified in info and places the result in byte.
*   For COM connections, reads a byte from the file handle specified in info and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {
	char c;
	int result;
	long ticks = GetTickCount();
	long sendTicks = ticks;

	while (1) {
		// send a message every few ms if canSend
		// independent of connection mode
		if (GetTickCount() - sendTicks >= 999) {
			if (canSend) {
				sendTicks = GetTickCount();

				handle_streaming_ports();
			}
		}

		if (connection_mode == MODE_TCP) {
			int *csock = (int *)info;
			fd_set sockset;
			TIMEVAL aTimeout;
			// wait until data arrives or a timeout occurs
			aTimeout.tv_sec = 0;
			aTimeout.tv_usec = 1000;		// one ms timeout

			// try to receive a byte within the timeout
			FD_ZERO(&sockset);
			FD_SET(*csock, &sockset);
			if ((result = select(0, &sockset, NULL, NULL, &aTimeout)) == SOCKET_ERROR) {
				printf("Network error: %d\n", WSAGetLastError());
				return OPDI_NETWORK_ERROR;
			}
			if (result == 0) {
				// ran into timeout
				// "real" timeout condition
				if (GetTickCount() - ticks >= timeout)
					return OPDI_TIMEOUT;
			} else {
				// socket contains data
				c = '\0';
				if ((result = recv(*csock, &c, 1, 0)) == SOCKET_ERROR) {
					return OPDI_NETWORK_ERROR;
				}
				// connection closed?
				if (result == 0)
					// dirty disconnect
					return OPDI_NETWORK_ERROR;

				// a byte has been received
				break;
			}
		}
		else
		if (connection_mode == MODE_COM) {
			HANDLE hndPort = (HANDLE)info;
			char inputData;
			DWORD bytesRead;
			int err;

			// first byte of connection remembered?
			if (first_com_byte != 0) {
				c = first_com_byte;
				first_com_byte = 0;
				break;
			}

			if (ReadFile(hndPort, &inputData, 1, &bytesRead, NULL) != 0) {
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
				err = GetLastError();
				// timeouts are expected here
				if (err != ERROR_TIMEOUT) {
					return OPDI_DEVICE_ERROR;
				}
			}
		}
	}

	*byte = (uint8_t)c;

	return OPDI_STATUS_OK;
}

/** For TCP connections, sends count bytes to the socket specified in info.
*   For COM connections, writes count bytes to the file handle specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	char *c = (char *)bytes;

	if (connection_mode == MODE_TCP) {
		int *csock = (int *)info;

		if (send(*csock, c, count, 0) == SOCKET_ERROR) {
			return OPDI_DEVICE_ERROR;
		}
	}
	else
	if (connection_mode == MODE_COM) {
		HANDLE hndPort = (HANDLE)info;
		DWORD length;

		if (WriteFile(hndPort, c, count, &length, NULL) == 0) {
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
	TCHAR nameBuf[1024];
	DWORD dwCompNameLen = 1024;

	if (opdi_slave_name == NULL) {
		opdi_slave_name = (char*)malloc(OPDI_SLAVE_NAME_LENGTH);
	}

	// requires #undef UNICODE to work properly
	if (0 != GetComputerName(nameBuf, &dwCompNameLen)) {
		sprintf((char *)opdi_slave_name, "WinOPDI (%s)", nameBuf);
	}
	else {
		opdi_slave_name = "WinOPDI";
	}

	// configure ports
	configure_ports();
}

/** This method handles an incoming TCP connection. It blocks until the connection is closed.
*/
int HandleTCPConnection(int *csock) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_TCP;
	init_device();

	// info value is the socket handle
	opdi_message_setup(&io_receive, &io_send, (void *)csock);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_start(&message, NULL, &my_protocol_callback);

	// release the socket
    free(csock);
    return result;
}

/** This method handles an incoming COM connection. It blocks until the connection is closed.
*/
int HandleCOMConnection(char firstByte, HANDLE hndPort) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_COM;
	first_com_byte = firstByte;
	init_device();

	// info value is the serial port handle
	opdi_message_setup(&io_receive, &io_send, (void *)hndPort);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_start(&message, NULL, &my_protocol_callback);

    return result;
}

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
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

// Implements device specific attributes and functions for the Linux OPDI slave implementation on a Raspberry Pi ("RaspOPDI").

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
#include "opdi_device.h"

#define OPDI_SLAVE_NAME_LENGTH 32
const char *opdi_slave_name;

char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char *opdi_encoding = "";
const char *opdi_encryption_method = "AES";
const char *opdi_supported_protocols = "BP";
const char *opdi_username = "admin";
const char *opdi_password = "admin";

uint16_t opdi_device_flags = OPDI_FLAG_AUTHENTICATION_REQUIRED;

uint16_t opdi_encryption_blocksize = OPDI_ENCRYPTION_BLOCKSIZE;
uint8_t opdi_encryption_buffer[OPDI_ENCRYPTION_BLOCKSIZE];
uint8_t opdi_encryption_buffer_2[OPDI_ENCRYPTION_BLOCKSIZE];

/// Ports

static struct opdi_Port digPort = { "DP1", "Digital Port" };
static struct opdi_Port anaPort = { "AP1", "Analog Port" };
static struct opdi_Port selectPort = { "SL1", "Switch" };
static struct opdi_Port dialPort = { "DL1", "Audio Volume" };
static struct opdi_DialPortInfo dialPortInfo = { 0, 100, 5 };
static struct opdi_Port streamPort1 = { "SP1", "Temp/Pressure" };
static struct opdi_StreamingPortInfo sp1Info = { "BMP085", OPDI_MESSAGE_PAYLOAD_LENGTH };

static char digmode[] = "0";		// floating input
static char digline[] = "0";

static char anamode[] = "1";		// output
static char anares[] = "1";		// 9 bit
static char anaref[] = "0";
static int anavalue = 0;

static const char *selectLabels[] = {"Position A", "Position B", "Position C", NULL};
static uint16_t selectPos = 0;

static int dialvalue = 0;

static double temperature = 20.0;
static double pressure = 1000.0;

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_SERIAL 2

static int connection_mode = 0;
static char first_com_byte = 0;

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
				// channel assigned for streaming port?
				if (sp1Info.channel > 0) {

					temperature += (rand() * 10.0 / RAND_MAX) - 5;
					if (temperature > 100) temperature = 100;
					if (temperature < -100) temperature = -100;
					pressure += (rand() * 10.0 / RAND_MAX) - 5;

					// send streaming message
					m.channel = sp1Info.channel;
					sprintf(bmp085, "BMP085:%.2f:%.2f", temperature, pressure);
					m.payload = bmp085;
					opdi_put_message(&m);

				}
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

// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	if (!strcmp(languages, "de_DE")) {
		digPort.name = "Digitaler Port";
		anaPort.name = "Analoger Port";
		selectPort.name = "Wahlschalter";
		dialPort.name = "Lautstärke";
		streamPort1.name = "Temperatur/Druck";
	}

	return OPDI_STATUS_OK;
}

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	if (!strcmp(port->id, anaPort.id)) {
		mode[0] = anamode[0];
		res[0] = anares[0];
		ref[0] = anaref[0];
		*value = anavalue;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	if (!strcmp(port->id, anaPort.id)) {
		anavalue = value;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	if (!strcmp(port->id, digPort.id)) {
		mode[0] = digmode[0];
		line[0] = digline[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, anaPort.id)) {
		anamode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]) {
	if (!strcmp(port->id, anaPort.id)) {
		anares[0] = res[0];
		// TODO check value in range
		anavalue = 0;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]) {

	if (!strcmp(port->id, anaPort.id)) {
		anaref[0] = ref[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]) {
	if (!strcmp(port->id, digPort.id)) {
		digline[0] = line[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, digPort.id)) {
		digmode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *position) {
	if (!strcmp(port->id, selectPort.id)) {
		*position = selectPos;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position) {
	if (!strcmp(port->id, selectPort.id)) {
		selectPos = position;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}


uint8_t opdi_get_dial_port_state(opdi_Port *port, int32_t *position) {
	if (!strcmp(port->id, dialPort.id)) {
		*position = dialvalue;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_dial_port_position(opdi_Port *port, int32_t position) {
/*
	HWAVEOUT i;
	int error;
	HWAVEOUT devCount = (HWAVEOUT)waveOutGetNumDevs();
	if (!strcmp(port->id, dialPort.id)) {

		// set volume of all wave devices
		for (i = 0; i < devCount; i++) {
			error = (waveOutSetVolume(i, (0xFFFF / 100) * position));
			if (error != MMSYSERR_NOERROR) 
				printf("Volume set error: &d", error);
		}

		dialvalue = position;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
*/		
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

	if (opdi_slave_name == NULL) {
		opdi_slave_name = (const char*)malloc(OPDI_SLAVE_NAME_LENGTH);
	}

	// requires #undef UNICODE to work properly
	if (0 != gethostname(nameBuf, compNameLen)) {
		sprintf((char *)opdi_slave_name, "RaspOPDI (%s)", nameBuf);
	}
	else {
		opdi_slave_name = "RaspOPDI";
	}

	// configure ports
	if (opdi_get_ports() == NULL) {
		digPort.type = OPDI_PORTTYPE_DIGITAL;
		digPort.caps = OPDI_PORTDIRCAP_BIDI;
		digPort.info.i = (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_HAS_PULLDN);
		opdi_add_port(&digPort);

		anaPort.type = OPDI_PORTTYPE_ANALOG;
		anaPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		anaPort.info.i = (OPDI_ANALOG_PORT_CAN_CHANGE_RES	
							   | OPDI_ANALOG_PORT_RESOLUTION_8	
							   | OPDI_ANALOG_PORT_RESOLUTION_9	
							   | OPDI_ANALOG_PORT_RESOLUTION_10	
							   | OPDI_ANALOG_PORT_RESOLUTION_11	
							   | OPDI_ANALOG_PORT_RESOLUTION_12	
							   | OPDI_ANALOG_PORT_CAN_CHANGE_REF	
							   | OPDI_ANALOG_PORT_REFERENCE_INT	
							   | OPDI_ANALOG_PORT_REFERENCE_EXT);
		opdi_add_port(&anaPort);

		selectPort.type = OPDI_PORTTYPE_SELECT;
		selectPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		selectPort.info.ptr = (void*)selectLabels;	// position labels
		opdi_add_port(&selectPort);

		dialPort.type = OPDI_PORTTYPE_DIAL;
		dialPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		dialPort.info.ptr = (void*)&dialPortInfo;
		opdi_add_port(&dialPort);

		streamPort1.type = OPDI_PORTTYPE_STREAMING;
		streamPort1.caps = OPDI_PORTDIRCAP_BIDI;
		streamPort1.info.ptr = (void*)&sp1Info;
		opdi_add_port(&streamPort1);
	}
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
	opdi_setup(&io_receive, &io_send, (void *)(long)csock);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	// initiate handshake
	result = opdi_start(&message, NULL, &my_protocol_callback);

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
	opdi_setup(&io_receive, &io_send, (void *)(long)fd);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	// initiate handshake
	result = opdi_start(&message, NULL, &my_protocol_callback);

	return result;
}

#ifdef __cplusplus
}
#endif 

uint8_t opdi_debug_msg(const uint8_t *str, uint8_t direction) {
	if (direction == OPDI_DIR_INCOMING)
		printf(">");
	else
		printf("<");
	printf("%s\n", str);
	return OPDI_STATUS_OK;
}

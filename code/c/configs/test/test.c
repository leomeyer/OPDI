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
    
// Test functions (platform independent)

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <WinBase.h>
#endif

#ifdef linux
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <errno.h>
#endif

#include "opdi_platformtypes.h"
#include "opdi_config.h"
#include "opdi_constants.h"
#include "opdi_ports.h"
#include "opdi_messages.h"
#include "opdi_slave_protocol.h"

const char *opdi_config_name;

char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char *opdi_encoding = "";
const char *opdi_encryption_method = "AES";
const char *opdi_supported_protocols = "BP,EP";
const char *opdi_username = "admin";
const char *opdi_password = "admin";

uint16_t opdi_device_flags = OPDI_FLAG_AUTHENTICATION_REQUIRED;

uint16_t opdi_encryption_blocksize = OPDI_ENCRYPTION_BLOCKSIZE;
const char *opdi_encryption_key = "0123456789012345";

// provide MS-specific encryption buffers
// (The MS compiler can't do dynamic array allocation on the stack apparently)
#ifdef _MSC_VER
uint8_t opdi_encryption_buffer[OPDI_ENCRYPTION_BLOCKSIZE];
uint8_t opdi_encryption_buffer_2[OPDI_ENCRYPTION_BLOCKSIZE];
#endif

/// Ports

static struct opdi_Port digPort = { "DP1", "Digital Port" };
static struct opdi_Port anaPort = { "AP1", "Analog Port" };
static struct opdi_Port selectPort = { "SL1", "Port selection" };
static struct opdi_Port dialPort = { "DL1", "Audio Volume" };
static struct opdi_DialPortInfo dialPortInfo = { 0, 100, 5 };
static struct opdi_Port testPort = { "TEST1", "Test cases" };
static struct opdi_Port streamPort1 = { "SP1", "Temp/Pressure" };
static struct opdi_StreamingPortInfo sp1Info = { "BMP085", OPDI_STREAMING_PORT_NORMAL };
static struct opdi_Port streamPort2 = { "SP2", "Clock" };
static struct opdi_StreamingPortInfo sp2Info = { "TEXT", OPDI_STREAMING_PORT_AUTOBIND };

static char digmode[] = OPDI_DIGITAL_MODE_INPUT_FLOATING;
static char digline[] = OPDI_DIGITAL_LINE_LOW;

static char anamode[] = "1";		// output
static char anares[] = "1";		// 9 bit
static char anaref[] = "0";
static int anavalue = 0;

static const char *selectLabels[] = {"Standard Ports", "Streaming Ports", NULL};
static uint16_t selectPos = 0;
static const char *testLabels[] = {"Normal", "Disconnect", "Crash", "Debug", "Error", NULL};
static uint16_t testPos = 0;

static int dialvalue = 0;

static double temperature = 20.0;
static double pressure = 1000.0;


// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	if (!strcmp(languages, "de_DE")) {
		digPort.name = "Digitaler Port";
		anaPort.name = "Analoger Port";
		selectPort.name = "Portauswahl";
		testPort.name = "Testfälle";
		dialPort.name = "Lautstärke";
		streamPort1.name = "Temperatur/Druck";
		streamPort2.name = "Uhrzeit";
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
	opdi_Port *refreshPort[2];
	if (!strcmp(port->id, anaPort.id)) {
		anavalue = value;

		// simulate crosstalk between pins
		// is the digital port configured as input?
		if (digmode[0] == OPDI_DIGITAL_MODE_INPUT_FLOATING[0]) {
			// the digital port is set to "on" if the analog value is greater than half the resolution maximum
			if (anavalue > (1 << (7 + anares[0] - '0')))
				digline[0] = OPDI_DIGITAL_LINE_HIGH[0];
			else
				digline[0] = OPDI_DIGITAL_LINE_LOW[0];
			// cause port refresh
			refreshPort[0] = &digPort;
			refreshPort[1] = NULL;
			opdi_refresh(refreshPort);
		}
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
	opdi_Port *refreshPort[2];
	if (!strcmp(port->id, digPort.id)) {
		digline[0] = line[0];

		// simulate crosstalk between pins
		// is the analog port configured as input?
		if (anamode[0] == '0') {
			// the analog port is set to the resolution maximum if the digital port is on
			if (digline[0] == '1')
				anavalue = (1 << (8 + anares[0] - '0')) - 1;
			else
				anavalue = 0;
			// cause port refresh
			refreshPort[0] = &anaPort;
			refreshPort[1] = NULL;
			opdi_refresh(refreshPort);
		}
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
	if (!strcmp(port->id, testPort.id)) {
		*position = testPos;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t position) {
	if (!strcmp(port->id, selectPort.id)) {
		// change?
		if (selectPos != position) {
			selectPos = position;
			// port selection has changed, remove ports
			opdi_clear_ports();
			// the select port is always needed
			opdi_add_port(&selectPort);
			// show default ports?
			if (selectPos == 0) {
				opdi_add_port(&digPort);
				opdi_add_port(&anaPort);
				opdi_add_port(&dialPort);
				opdi_add_port(&testPort);
			} else {
				// show streaming ports
				opdi_add_port(&streamPort1);
				opdi_add_port(&streamPort2);
			}

			// ask the master to reconfigure
			opdi_reconfigure();
		}
	} else
	if (!strcmp(port->id, testPort.id)) {
		switch(position) {
			// simulate test cases
		case 1: return opdi_disconnect();
		case 2: exit(1);	// crash
		case 3: return opdi_send_debug("Test debug message");
		case 4: return OPDI_DEVICE_ERROR;
		}
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
	if (!strcmp(port->id, dialPort.id)) {
		dialvalue = position;
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

void configure_ports(void) {
	if (opdi_get_ports() == NULL) {
		digPort.type = OPDI_PORTTYPE_DIGITAL;
		digPort.caps = OPDI_PORTDIRCAP_BIDI;
		digPort.info.i = (OPDI_DIGITAL_PORT_HAS_PULLUP | OPDI_DIGITAL_PORT_HAS_PULLDN);

		anaPort.type = OPDI_PORTTYPE_ANALOG;
		anaPort.caps = OPDI_PORTDIRCAP_BIDI;
		anaPort.info.i = (OPDI_ANALOG_PORT_CAN_CHANGE_RES	
							   | OPDI_ANALOG_PORT_RESOLUTION_8	
							   | OPDI_ANALOG_PORT_RESOLUTION_9	
							   | OPDI_ANALOG_PORT_RESOLUTION_10	
							   | OPDI_ANALOG_PORT_RESOLUTION_11	
							   | OPDI_ANALOG_PORT_RESOLUTION_12	
							   | OPDI_ANALOG_PORT_CAN_CHANGE_REF	
							   | OPDI_ANALOG_PORT_REFERENCE_INT	
							   | OPDI_ANALOG_PORT_REFERENCE_EXT);

		selectPort.type = OPDI_PORTTYPE_SELECT;
		selectPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		selectPort.info.ptr = (char *)selectLabels;	// position labels

		testPort.type = OPDI_PORTTYPE_SELECT;
		testPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		testPort.info.ptr = (char *)testLabels;		// test case labels

		dialPort.type = OPDI_PORTTYPE_DIAL;
		dialPort.caps = OPDI_PORTDIRCAP_OUTPUT;
		dialPort.info.ptr = &dialPortInfo;

		streamPort1.type = OPDI_PORTTYPE_STREAMING;
		streamPort1.info.ptr = &sp1Info;

		streamPort2.type = OPDI_PORTTYPE_STREAMING;
		streamPort2.info.ptr = &sp2Info;

		// on init, only the default ports are visible
		// the select port lets the user switch to streaming ports and back
		selectPos = 0;
		opdi_add_port(&selectPort);
		opdi_add_port(&digPort);
		opdi_add_port(&anaPort);
		opdi_add_port(&dialPort);
		opdi_add_port(&testPort);
//		opdi_add_port(&streamPort1);
	}
}

void handle_streaming_ports() {
	opdi_Message m;
	char bmp085[32];
	char clocktext[32];
	time_t mytime = time(NULL);

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
	// channel assigned for streaming port?
	if (sp2Info.channel > 0) {

		// send streaming message
		m.channel = sp2Info.channel;
		sprintf(clocktext, "%s", ctime(&mytime));
		// remove trailing 0x0A (it's the message separator and will prevent the message from being sent)
		clocktext[strlen(clocktext) - 1] = '\0';
		m.payload = clocktext;
		opdi_put_message(&m);
	}
}

uint8_t opdi_debug_msg(const uint8_t *str, uint8_t direction) {
	if (direction == OPDI_DIR_INCOMING)
		printf(">");
	else
		printf("<");
	printf("%s\n", str);
	return OPDI_STATUS_OK;
}


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

#include <string.h>

#include "opdi_platformtypes.h"
#include "opdi_platformfuncs.h"
#include "opdi_constants.h"
#include "opdi_messages.h"
#include "opdi_protocol.h"
#include "opdi_device.h"

#include "common.h"
#include "debug.h"
#include "format/format.h"
#include "aes/AES_driver.h"
#include "nmeagen/nmeagen.h"
#include "timer0/timer0.h"
#include "bmp085/bmp085.h"
#include "chr6dm/chr6dm.h"

// Device settings

const char *opdi_slave_name = "iControl";
char opdi_master_name[OPDI_MASTER_NAME_LENGTH];
const char *opdi_encoding = "";
const char *opdi_encryption_method = "AES";
uint16_t opdi_encryption_blocksize = AES_BLOCK_LENGTH;
const char *opdi_supported_protocols = "BP";
uint16_t opdi_device_flags = 0;
const char *opdi_username = "";
const char *opdi_password = "";

static uint8_t *key = (uint8_t *)"0123456789012345";

static long update_ticks;


/// Port definitions

static struct opdi_Port digPort1 = { "DP1", "Digital Port" };
static struct opdi_Port anaPort1 = { "AP1", "Analog Port" };
static struct opdi_Port bmp085Port = { "SP1", "Temp/Pressure" };
static struct opdi_StreamingPortInfo bmp085Info = { "BMP085", OPDI_MESSAGE_PAYLOAD_LENGTH };
static struct opdi_Port nmeaPort = { "SP2", "GPS" };
static struct opdi_StreamingPortInfo nmeaInfo = { "NMEAGen", OPDI_MESSAGE_PAYLOAD_LENGTH };
static struct opdi_Port ahrsPort = { "SP3", "AHRS" };
static struct opdi_StreamingPortInfo ahrsInfo = { "CHR-6dm", OPDI_MESSAGE_PAYLOAD_LENGTH };

static char dig1mode[] = "1";
static char dig1line[] = "0";

static char ana1mode[] = "1";
static char ana1res[] = "1";
static char ana1ref[] = "0";
static int32_t ana1value = 0;

static const char *chr6dm_cal = "cal";

static void led_on(void) {
	PORTE.OUTCLR = PIN7_bm;
}

static void led_off(void) {
	PORTE.OUTSET = PIN7_bm;	
}

// is called by the protocol
uint8_t opdi_choose_language(const char *languages) {
	// change port names and/or device name here (optional)

	return OPDI_STATUS_OK;
}

uint8_t opdi_get_analog_port_state(opdi_Port *port, char mode[], char res[], char ref[], int32_t *value) {
	if (!strcmp(port->id, anaPort1.id)) {
		mode[0] = ana1mode[0];
		res[0] = ana1res[0];
		ref[0] = ana1ref[0];
		*value = ana1value;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_value(opdi_Port *port, int32_t value) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1value = value;
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_digital_port_state(opdi_Port *port, char mode[], char line[]) {
	if (!strcmp(port->id, digPort1.id)) {
		mode[0] = dig1mode[0];
		line[0] = dig1line[0];
	} else
		// unknown analog port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}


uint8_t opdi_set_analog_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1mode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_resolution(opdi_Port *port, const char res[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1res[0] = res[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_analog_port_reference(opdi_Port *port, const char ref[]) {
	if (!strcmp(port->id, anaPort1.id)) {
		ana1ref[0] = ref[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_line(opdi_Port *port, const char line[]) {
	if (!strcmp(port->id, digPort1.id)) {
		dig1line[0] = line[0];
		// output?
		if (dig1mode[0] == '3')
			if (dig1line[0] == '1') {
				led_on();
			} else {
				led_off();
			}
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_digital_port_mode(opdi_Port *port, const char mode[]) {
	if (!strcmp(port->id, digPort1.id)) {
		dig1mode[0] = mode[0];
	} else
		// unknown port
		return OPDI_PORT_UNKNOWN;
		
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_select_port_state(opdi_Port *port, uint16_t *pos) {
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_select_port_position(opdi_Port *port, uint16_t pos) {
	return OPDI_STATUS_OK;
}

uint8_t opdi_get_dial_port_state(opdi_Port *port, int32_t *position) {
	return OPDI_STATUS_OK;
}

uint8_t opdi_set_dial_port_position(opdi_Port *port, int32_t position) {
	return OPDI_STATUS_OK;
}


/** CHR-6dm communication */
static uint8_t chr6dm_DataReceived(opdi_Port *port, const char *data) {
	// calibrate CHR-6dm?
	if (port == &ahrsPort && !strcmp(data, chr6dm_cal)) {
		// let the driver calibrate
		return chr6dm_calibrate();
	}
	return OPDI_STATUS_OK;
}

uint8_t device_init(void) {
	
	if (!nmeagen_init())  {
		debug_str("nmea init problem");
	} else {
		// tell the GPS to send only GPGGA sentences, two per second
		// this avoids an overflow of the serial input buffer causing message corruption
		nmeagen_send_command("$PMTK314,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0*2D\r\n");
	}
		
	if (!chr6dm_init())  {
		debug_str("chr6dm init problem");
	} else {
//		nmeagen_send_command("$PMTK314,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0*2D\r\n");
	}
		
	if (!bmp085_init()) {
		debug_str("BMP085 init problem");
	}
	
	// configure ports
	if (opdi_get_ports() == NULL) {
		digPort1.type = OPDI_PORTTYPE_DIGITAL;
		digPort1.caps = OPDI_PORTDIRCAP_BIDI;
		digPort1.info.i = 1;		// HAS_PULLUP
		opdi_add_port(&digPort1);

		anaPort1.type = OPDI_PORTTYPE_ANALOG;
		anaPort1.caps = OPDI_PORTDIRCAP_OUTPUT;
		opdi_add_port(&anaPort1);
		
		bmp085Port.type = OPDI_PORTTYPE_STREAMING;
		bmp085Port.caps = OPDI_PORTDIRCAP_BIDI;
		bmp085Port.info.ptr = &bmp085Info;
		if (bmp085_present()) {
			opdi_add_port(&bmp085Port);
		}
		
		nmeaPort.type = OPDI_PORTTYPE_STREAMING;
		nmeaPort.caps = OPDI_PORTDIRCAP_BIDI;
		nmeaPort.info.ptr = &nmeaInfo;
		opdi_add_port(&nmeaPort);		
		
		ahrsPort.type = OPDI_PORTTYPE_STREAMING;
		ahrsPort.caps = OPDI_PORTDIRCAP_BIDI;
		ahrsPort.info.ptr = &ahrsInfo;
		ahrsInfo.dataReceived = &chr6dm_DataReceived;
		if (chr6dm_ready())
			opdi_add_port(&ahrsPort);		
	}
	
	// everything ok
	led_off();
	
	return OPDI_STATUS_OK;
}

/// AES support

uint8_t opdi_encrypt_block(uint8_t *dest, const uint8_t *src) {
	// taken from Atmel's AppNote AVR1318

	AES_software_reset();
	
	if (AES_encrypt((uint8_t *)src, (uint8_t *)dest, key))
		return OPDI_STATUS_OK;
		
	return OPDI_ENCRYPTION_ERROR;
}

uint8_t opdi_decrypt_block(uint8_t *dest, const uint8_t *src) {
	// taken from Atmel's AppNote AVR1318
	uint8_t lastsubkey[opdi_encryption_blocksize];

	AES_software_reset();
	AES_lastsubkey_generate(key, lastsubkey);
	
	if (AES_decrypt((uint8_t *)src, dest, lastsubkey))
		return OPDI_STATUS_OK;
		
	return OPDI_ENCRYPTION_ERROR;
}

static void protocol_callback(uint8_t state) {
	
	// TODO status LED	
}

/** Starts a connection by processing messages. */
uint8_t device_start(void) {
	
	opdi_Message message;
	uint8_t result;
	
	debug_str("START\n");
	
	while (1) {

		// prepare the protocol layer for a new connection
		result = opdi_init();
		if (result != OPDI_STATUS_OK) {
			return result;
		}
		
		result = opdi_get_message(&message, OPDI_CANNOT_SEND);
		if (result != OPDI_STATUS_OK) {
			return result;
		}

		// initiate handshake
		result = opdi_start(&message, NULL, &protocol_callback);
		if (result != OPDI_STATUS_OK) {
			return result;
		}
	}

	update_ticks = timer0_ticks();
	
	return result;
}

/** Sends streaming data from the drivers if possible.
*/
uint8_t device_serve_streams(void) {
	
#define UPDATE_MS	500
	opdi_Message m;
	char buffer[128];
	int16_t length;
	int32_t temp;
	int32_t pressure;
	const char *nmea_msg;

	// CHR-6dm
	uint8_t data[64];
	uint16_t pos;
	uint16_t sum;
	uint16_t value;
	int16_t *val = (int16_t *)&value;
	uint8_t numsens = 0;
	uint8_t error = 0;
	
	// wait until update period expired
	if (timer0_ticks() - update_ticks < UPDATE_MS) return OPDI_STATUS_OK;
	update_ticks = timer0_ticks();

	// channel assigned for BMP085 port?
	if (bmp085Info.channel > 0) {

		// read temperature and pressure
		if (bmp085GetRealValues(&temp, &pressure)) {
			strcpy(buffer, "BMP085:");
//			temp = 198;
			fstr_dec(temp, 5, 1, '.', buffer + 7);
			buffer[15] = ':';
//			pressure = 9720;
			fstr_dec(pressure, 5, 1, '.', buffer + 16);
			// send streaming message with BMP085 data
			m.channel = bmp085Info.channel;
			m.payload = buffer;
			opdi_put_message(&m);			
		} else {
			debug_str("bmp085 error");
		}
	}

	// channel assigned for GPS port?
	if (nmeaInfo.channel > 0) {

		// GPS data available?		
		while (nmeagen_has_message()) {
			nmea_msg = nmeagen_get_message();
			// send only GPGGA messages
			if (!strncmp(nmea_msg, "$GPGGA", 6)) {
				// send streaming message with GPS data
				m.channel = nmeaInfo.channel;
				m.payload = (char*)nmea_msg;
				opdi_put_message(&m);
				
				// clean remaining messages
				while (nmeagen_has_message()) nmeagen_get_message();
			}			
		}
	}

	// channel assigned for AHRS port?
	if (ahrsInfo.channel > 0) {

		length = chr6dm_get_data(data);
		if (length > 0) {

#define ERROR_NODATA	1	// no or not enough data
#define ERROR_EODATA	2	// unexpected end of data
#define ERROR_CHKSUM	3	// checksum is wrong

			// minimum length for a valid message
			if (length < 11) {
				error = ERROR_NODATA;
				goto done;
			}

			// count number of sensors (set bits in data[5..6])
			sum = (data[5] << 8) + data[6];
			while (sum)
			{
				numsens = numsens + (sum & 1);
				sum = sum >> 1;
			}

			if (numsens == 0) {
				error = ERROR_NODATA;
				goto done;
			}

			for (pos = 0; pos < 7; pos++)
				sum += data[pos];

			// write sensor value to buffer
			value = (data[5] << 8) + data[6];
			fstr_dec((int)*val, 5, 0, '.', &buffer[0]);
			
			// convert each sensor's value to a string and format it to the buffer
			for (pos = 0; pos < numsens; pos++) {
				if (pos * 2 + 7 > length) {
					error = ERROR_EODATA;
					goto done;
				}
				value = (data[pos * 2 + 7] << 8) + data[pos * 2 + 8];
				fstr_dec((int)*val, 5, 0, '.', &buffer[(pos + 1) * 6]);
				sum += data[pos * 2 + 7] + data[pos * 2 + 8];
			}

			// checksum
			value = (data[pos * 2 + 7] << 8) + data[pos * 2 + 8];
			if (value != sum)
				error = ERROR_CHKSUM;

		done:
			if (error != 0) {
				strcpy(buffer, "Error: ");
				fstr_dec(error, 1, 0, '.', &buffer[7]);
			} else {
				// send streaming message
				m.channel = ahrsInfo.channel;
				m.payload = (char*)buffer;
				opdi_put_message(&m);
			}
		}	// AHRS data available
	}	// AHRS assigned
	
	return OPDI_STATUS_OK;
}
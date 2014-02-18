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

// Driver for pressure sensor BMP085

/*
 * Based on:     BMP085 Test Code
	April 7, 2010
	by: Jim Lindblom
* and: AVR TWI Example code
 */

#define F_CPU	F_OSC
#include "util/delay.h"
#include "avr_compiler.h"
#include "bmp085.h"
#include "opdi_device.h"
#include "debug.h"
#include "format/format.h"
#include "timer0/timer0.h"

#define OVERSAMPLING	0

#define TRUE	1
#define FALSE	0


// specify whether to use TWI (hardware) or I2C (software)
// uncomment to use I2C
#define USE_TWI


static uint8_t bmp085_found;

// calibration values
static int16_t ac1;
static int16_t ac2; 
static int16_t ac3; 
static uint16_t ac4;
static uint16_t ac5;
static uint16_t ac6;
static int16_t b1; 
static int16_t b2;
static int16_t mb;
static int16_t mc;
static int16_t md;



static void debug_value(char* name, int32_t val) {
	char *buffer = "================";
	uint16_t i = 0;

	while (name[i]) {
		buffer[i] = name[i];
		i++;
	}	
	
	fstr_dec(val, 6, 0, '.', buffer + i);
	debug_str(buffer);
	debug_str("\r\n");
	_delay_ms(100);
}



#ifdef USE_TWI

	#include "twi/twi_master_driver.h"
	#define I2C_BUS		&TWIE
	#define I2C_PORT	PORTE

	/*! Address of BMP085 on I2C bus. */
	#define ADDRESS    (0xEE >> 1)

	/*! BAUDRATE 100kHz */
	#define BAUDRATE	400000
	#define TWI_BAUDSETTING TWI_BAUD(F_OSC, BAUDRATE)

	TWI_Master_t twiMaster;    /*!< TWI master module. */

	/*! TWIC Master Interrupt vector. */
	ISR(TWIE_TWIM_vect)
	{
		TWI_MasterInterruptHandler(&twiMaster);
	}
	
#else
	#include "i2c/i2cmaster.h"

	/*! Address of BMP085 on I2C bus. */
	#define ADDRESS    0xEE
#endif


uint8_t bmp085_init(void) {
#define debug	1
	// startup delay
	_delay_ms(10);
	
	bmp085_found = 0;
	
#ifdef USE_TWI
	/* Initialize TWI master. */
	TWI_MasterInit(&twiMaster,
	               I2C_BUS,
	               TWI_MASTER_INTLVL_LO_gc,
	               TWI_BAUDSETTING);
	
#else
	// configure virtual port VPORT0
	// see i2cmaster.S
	PORTCFG.VPCTRLA = PORTCFG_VP0MAP_PORTE_gc;

	// init I2C interface	
    i2c_init();

#endif

	// read calibration values
	if (debug) debug_str("\r\n");
//	while (1)
		if (!bmp085ReadShort(0xAA, &ac1)) return 0;
	if (debug) debug_value("ac1", ac1);
	if (!bmp085ReadShort(0xAC, &ac2)) return 0;
	if (debug) debug_value("ac2", ac2);
	if (!bmp085ReadShort(0xAE, &ac3)) return 0;
	if (debug) debug_value("ac3", ac3);
	if (!bmp085ReadShort(0xB0, (int16_t *)&ac4)) return 0;
	if (debug) debug_value("ac4", ac4);
	if (!bmp085ReadShort(0xB2, (int16_t *)&ac5)) return 0;
	if (debug) debug_value("ac5", ac5);
	if (!bmp085ReadShort(0xB4, (int16_t *)&ac6)) return 0;
	if (debug) debug_value("ac6", ac6);
	if (!bmp085ReadShort(0xB6, &b1)) return 0;
	if (debug) debug_value("b1", b1);
	if (!bmp085ReadShort(0xB8, &b2)) return 0;
	if (debug) debug_value("b2", b2);
	if (!bmp085ReadShort(0xBA, &mb)) return 0;
	if (debug) debug_value("mb", mb);
	if (!bmp085ReadShort(0xBC, &mc)) return 0;
	if (debug) debug_value("mc", mc);
	if (!bmp085ReadShort(0xBE, &md)) return 0;
	if (debug) debug_value("md", md);
   	 
	bmp085_found = 1;
	
	return 1;
}

uint8_t bmp085_present() {
	return bmp085_found;
}
		
// bmp085ReadShort will read two sequential 8-bit registers, and return a 16-bit value
// the MSB register is read first
// Input: First register to read
// Output: 16-bit value of (first register value << 8) | (sequential register value)
uint8_t bmp085ReadShort(uint8_t reg, int16_t *result)
{
#ifdef USE_TWI
 	uint8_t i2cdata[1];
	char msb, lsb;

	// send register address
	i2cdata[0] = reg;
	TWI_MasterWriteRead(&twiMaster, ADDRESS, &i2cdata[0], 1, 2);		
	
	while (twiMaster.status != TWIM_STATUS_READY) {
		/* Wait until transaction is complete. */
	}
	
	if (twiMaster.result != TWIM_RESULT_OK)
		// error
		return 0;
	
	msb = twiMaster.readData[0];
	lsb = twiMaster.readData[1];
	
	*result = msb << 8;
	*result |= lsb;
	
	return 1;
#else

    unsigned char ret;
	char msb;
	char lsb;

    ret = i2c_start(ADDRESS + I2C_WRITE);       // set device address and write mode
    if ( ret ) {
        /* failed to issue start condition, possibly no device found */
        i2c_stop();
        return 0;	// error
    } else {
        i2c_write(reg);
        i2c_rep_start(ADDRESS + I2C_READ);       // set device address and read mode
        msb = i2c_readAck();                    // read one byte
        lsb = i2c_readNak();                    // read one byte
        i2c_stop();
		*result = (msb << 8) + lsb;
		return 1;	// ok
	}	

#endif
}


uint8_t bmp085ReadTemp(int32_t *temp)
{
	uint8_t result;
	uint16_t t;

#ifdef USE_TWI
 	uint8_t i2cdata[2];
	i2cdata[0] = 0xF4;
	i2cdata[1] = 0x2E;

	TWI_MasterWrite(&twiMaster, ADDRESS, &i2cdata[0], 2);		

	// Wait until transaction is complete
	while (twiMaster.status != TWIM_STATUS_READY) {
	}

	if (twiMaster.result != TWIM_RESULT_OK)
		// error
		return 0;

#else
    unsigned char ret;

    ret = i2c_start(ADDRESS + I2C_WRITE);       // set device address and write mode
    if ( ret ) {
        /* failed to issue start condition, possibly no device found */
        i2c_stop();
        return 0;	// error
    } else {
        /* issuing start condition ok, device accessible */
        i2c_write(0xF4); 
        i2c_write(0x2E);
        i2c_stop();                            // set stop conditon = release bus
	}	

#endif

	timer0_delay(5);	// max time is 4.5ms
	
	result = bmp085ReadShort(0xF6, (int16_t *)&t);
	*temp = t;
	return result;
}


uint8_t bmp085ReadPressure(int32_t *pressure)
{
	int32_t p;
	
#ifdef USE_TWI
	
 	uint8_t i2cdata[2];

	i2cdata[0] = 0xF4;
	i2cdata[1] = 0x34 + (OVERSAMPLING << 6);
	 
	TWI_MasterWrite(&twiMaster, ADDRESS, &i2cdata[0], 2);
	
	while (twiMaster.status != TWIM_STATUS_READY) {
		// Wait until transaction is complete.
	}
		
	if (twiMaster.result != TWIM_RESULT_OK)
		// error
		return 0;
	
	timer0_delay(5 * (OVERSAMPLING + 1));

	// send register address
	i2cdata[0] = 0xF6;
	TWI_MasterWriteRead(&twiMaster, ADDRESS, &i2cdata[0], 1, 3);		
	
	while (twiMaster.status != TWIM_STATUS_READY) {
		/* Wait until transaction is complete. */
	}
	
	if (twiMaster.result != TWIM_RESULT_OK)
		// error
		return 0;
		
	p = ((int32_t)twiMaster.readData[0] << 16);
	p += ((int32_t)twiMaster.readData[1] << 8);
	p += (int32_t)twiMaster.readData[2];
		
	*pressure = p >> (8 - OVERSAMPLING);
	return 1;
#else
    unsigned char ret;
	char msb;
	char lsb;
	char xlsb;

    ret = i2c_start(ADDRESS + I2C_WRITE);       // set device address and write mode
    if ( ret ) {
        /* failed to issue start condition, possibly no device found */
        i2c_stop();
        return 0;	// error
    } else {
        /* issuing start condition ok, device accessible */
        i2c_write(0xF4); 
        i2c_write(0x34 + (6 << OVERSAMPLING));
        i2c_stop();                            // set stop conditon = release bus
	}	

	timer0_delay(5 * (OVERSAMPLING + 1));
	
    ret = i2c_start(ADDRESS + I2C_WRITE);       // set device address and write mode
    if ( ret ) {
        /* failed to issue start condition, possibly no device found */
        i2c_stop();
        return 0;	// error
    } else {
        i2c_write(0xF6);
        i2c_rep_start(ADDRESS + I2C_READ);       // set device address and read mode
        msb = i2c_readAck();                    // read one byte
        lsb = i2c_readAck();                    // read one byte
        xlsb = i2c_readNak();                    // read one byte
        i2c_stop();
		*pressure = (((int32_t)msb << 16) + ((int32_t)lsb << 8) + xlsb) >> (8 - OVERSAMPLING);
		return 1;	// ok
	}
	
#endif
}

uint8_t bmp085GetRealValues(int32_t *temperature, int32_t *pressure)
{
	int32_t ut;
	int32_t up;
	int32_t x1, x2, b5, b6, x3, b3, p;
	uint32_t b4, b7;
	
	// some bug here, have to read twice to get good data
	if (!bmp085ReadTemp(&ut)) return 0;
	if (!bmp085ReadTemp(&ut)) return 0;
	if (!bmp085ReadPressure(&up)) return 0;
	if (!bmp085ReadPressure(&up)) return 0;

    x1 = (((int32_t) ut - ac6) * ac5) >> 15;
    x2 = ((int32_t) mc << 11) / (x1 + md);
    b5 = x1 + x2;
	*temperature = (b5 + 8) >> 4;  // temperature in 0.1°C
	
	b6 = b5 - 4000;
	x1 = (b2 * (b6 * b6 >> 12)) >> 11;
	x2 = ac2 * b6 >> 11;
	x3 = x1 + x2;
	b3 = ((((int32_t) ac1 * 4 + x3) << OVERSAMPLING) + 2) / 4;
	x1 = ac3 * b6 >> 13;
	x2 = (b1 * (b6 * b6 >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (ac4 * (uint32_t) (x3 + 32768)) >> 15;
	b7 = ((uint32_t) up - b3) * (50000 >> OVERSAMPLING);
	p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	*pressure = p + ((x1 + x2 + 3791) >> 4);
	
	return 1;
}


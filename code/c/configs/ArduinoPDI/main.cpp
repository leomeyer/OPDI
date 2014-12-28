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

/** OPDI example program for Arduino
 * Requires serial communication between master and slave, preferably via Bluetooth.
 * Provides a few example ports.
 */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
//#include "memory.h"

#include <opdi_constants.h>

#include "ArduinOPDI.h"

// define global OPDI instance
OPDI Opdi = ArduinOPDI();

// Port definitions

// Digital port configurable as input and output (input with pullup)
OPDI_DigitalPortPin digPort1 = OPDI_DigitalPortPin("DP1", "Pin 12", OPDI_PORTDIRCAP_BIDI, OPDI_DIGITAL_PORT_HAS_PULLUP, 12);
// Digital port connected to LED (on most Arduino boards), output only
OPDI_DigitalPortPin digPort2 = OPDI_DigitalPortPin("DP2", "Pin 13", OPDI_PORTDIRCAP_OUTPUT, 0, 13);

// Analog input port
OPDI_AnalogPortPin anaPort1 = OPDI_AnalogPortPin("AP1", "Pin A0", OPDI_PORTDIRCAP_INPUT, OPDI_ANALOG_PORT_CAN_CHANGE_REF, A0);
// Analog output port (PWM)
OPDI_AnalogPortPin anaPort2 = OPDI_AnalogPortPin("AP2", "PWM 11", OPDI_PORTDIRCAP_OUTPUT, 0, 11);

int main(void)
{
	init();

	setup();

	for (;;)
		loop();

	return 0;
}

void setup() {                
	// initialize the digital pin as an output.
	// Pin 13 has an LED connected on most Arduino boards
	pinMode(13, OUTPUT);

	// start serial port at 9600 baud
	Serial.begin(9600);

	// initialize the OPDI system
	Opdi.setup("ArduinOPDI");
	Opdi.setIdleTimeout(20000);

	// add the ports provided by this configuration
	Opdi.addPort(&digPort1);
	Opdi.addPort(&digPort2);
	Opdi.addPort(&anaPort1);
	Opdi.addPort(&anaPort2);
}

/* This function can be called to perform regular housekeeping.
* Passing it to the Opdi.start() function ensures that it is called regularly.
* The function can send OPDI messages to the master or even disconnect a connection.
* It should always return OPDI_STATUS_OK in case everything is normal.
* If disconnected it should return OPDI_DISCONNECTED.
* Any value that is not OPDI_STATUS_OK will terminate an existing connection.
*/
uint8_t doWork() {
	// nothing to do - just an example
	return OPDI_STATUS_OK;
}

void loop() {

	// do housekeeping
	doWork();

	// character received on serial port? may be a connection attempt
	if (Serial.available() > 0) {

		// indicate connected status
		digitalWrite(13, HIGH);   // set the LED on

		// start the OPDI protocol, passing a pointer to the housekeeping function
		// this call blocks until the slave is disconnected
		uint8_t result = Opdi.start(&doWork);

		// no regular disconnect?
		if (result != OPDI_DISCONNECTED) {
			digitalWrite(13, LOW);    // set the LED off
			delay(500);

			// flash error code on LED
			for (uint8_t i = 0; i < result; i++) {
				digitalWrite(13, HIGH);   // set the LED on
				delay(200);              // wait
				digitalWrite(13, LOW);    // set the LED off
				delay(200);              // wait
			}
		}

		digitalWrite(13, LOW);    // set the LED off
	}
}


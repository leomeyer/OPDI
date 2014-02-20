#include <WProgram.h>
#include <arduino.h>

#include <opdi_constants.h>
#include <opdi_ports.h>

#include <opdi_configspecs.h>

#include "OPDI.h"

OPDI_DigitalPortPin digPort1 = OPDI_DigitalPortPin("DP1", "Pin 12", OPDI_PORTDIRCAP_BIDI, OPDI_DIGITAL_PORT_HAS_PULLUP, 12);
OPDI_DigitalPortPin digPort2 = OPDI_DigitalPortPin("DP2", "Pin 13", OPDI_PORTDIRCAP_OUTPUT, 0, 13);

OPDI_AnalogPortPin anaPort1 = OPDI_AnalogPortPin("AP1", "Pin A0", OPDI_PORTDIRCAP_INPUT, OPDI_ANALOG_PORT_CAN_CHANGE_REF, A0);
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

void loop() {

	// character received on serial port?
	if (Serial.available() > 0) {

		// indicate connected status
		digitalWrite(13, HIGH);   // set the LED on

		// start the OPDI protocol
		uint8_t result = Opdi.start();

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


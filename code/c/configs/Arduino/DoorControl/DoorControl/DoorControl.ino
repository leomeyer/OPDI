
//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2015 Leo Meyer (leo@leomeyer.de)
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

/** DoorControl for Arduino
 * - Keypad
 * - OPDI slave implementation
 */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <opdi_constants.h>

#include "ArduinOPDI.h"
#include <Keypad.h>
#include <EEPROM.h>

// keyboard input state
uint32_t enteredValue = 0;
int64_t codeValue = 0;    // is read from the EEPROM at startup and modified by the dial port

// Special port definitions

// EEPROM dial port: a 10 byte EEPROM sequence starting at the specified address
// The first 8 bytes are the 64 bit value (LSB first). The next two bytes are the checksum.
// If the checksum is not valid the position returned will be 0.
class OPDI_EEPROMDialPort : public OPDI_DialPort {
  
protected:
  int address;
  
public:
OPDI_EEPROMDialPort(const char *id, const char *label, const int address, const int64_t maxValue, char *extendedInfo) : 
    OPDI_DialPort(id, label, 0, maxValue, 1, 0) {
   // this->port.extendedInfo = extendedInfo;     
}

virtual ~OPDI_EEPROMDialPort() {}

virtual uint8_t setPosition(int64_t position) {
  int checksum = 0;
  int64_t value = position;
  opdi_DialPortInfo *portInfo = (opdi_DialPortInfo *)this->port.info.ptr;
  if (value > portInfo->max)
    value = portInfo->max;
  if (value < portInfo->min)
    value = portInfo->min;
  for (int i = 0; i < 8; i++) {
    byte val = value & 0xff;
    EEPROM.write(this->address + i, val);
    checksum += val;
    value >>= 8;
  }
  // write checksum
  EEPROM.write(this->address + 8, (byte)(checksum & 0xff));
  EEPROM.write(this->address + 9, (byte)(checksum >> 8));
  
  codeValue = position;
  
  return OPDI_STATUS_OK;
}

virtual uint8_t getState(int64_t *position) {
  int checksum = 0;
  int64_t value = 0;
  *position = 0;  // default
  opdi_DialPortInfo *portInfo = (opdi_DialPortInfo *)this->port.info.ptr;
  for (int i = 0; i < 8; i++) {
    int64_t val = EEPROM.read(this->address + i);
    value += val << (i * 8);
    checksum += val;
  }
  // compare checksum
  if ((EEPROM.read(this->address + 8) == (byte)(checksum & 0xff))
      && (EEPROM.read(this->address + 9) == (byte)(checksum >> 8)))
          *position = value;
  if (*position > portInfo->max)
    *position = portInfo->max;
  if (*position < portInfo->min)
    *position = portInfo->min;
  return OPDI_STATUS_OK;
}

};

// The ArduinOPDI class defines all the Arduino specific stuff like serial ports communication.
ArduinOPDI ArduinOpdi = ArduinOPDI();
// define global OPDI instance
// important: for polymorphism to work, assign own OPDI instance
OPDI* Opdi = &ArduinOpdi;

// Port definitions

OPDI_EEPROMDialPort eeprom1 = OPDI_EEPROMDialPort("EDP1", "Code", 0, 99999, "unit=keypadCode");

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'7','8','9','E'},
  {'4','5','6','Y'},
  {'1','2','3','D'},
  {'0','.','-','R'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 9, 10, 11}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

////////////////

uint8_t checkerror(uint8_t result) {
	if (result != OPDI_STATUS_OK) {
		digitalWrite(13, LOW);    // set the LED off
		delay(500);

		// flash error code on LED
		for (uint8_t i = 0; i < result; i++) {
			digitalWrite(13, HIGH);   // set the LED on
			delay(200);              // wait
			digitalWrite(13, LOW);    // set the LED off
			delay(200);              // wait
		}
		digitalWrite(13, LOW);    // set the LED off
		return 0;
	}
	return 1;
}

uint8_t setupDevice() {
	// initialize the digital pin as an output.
	// Pin 13 has an LED connected on most Arduino boards
	pinMode(13, OUTPUT);

	// start serial port at 9600 baud
	Serial.begin(9600);

	// initialize the OPDI system
	uint8_t result = ArduinOpdi.setup("DoorControl", 60000);  // one minute timeout
	if (checkerror(result) == 0)
		return 0;

	// add the ports provided by this configuration
        Opdi->addPort(&eeprom1);
        
        // read keypad code value from EEPROM
        eeprom1.getState(&codeValue);

	return 1;
}

int main(void)
{
	init();

	uint8_t setupOK = setupDevice();

	for (;;)
		if (setupOK)
			loop();

	return 0;
}

/* This function can be called to perform regular housekeeping.
* Passing it to the Opdi->start() function ensures that it is called regularly.
* The function can send OPDI messages to the master or even disconnect a connection.
* It should always return OPDI_STATUS_OK in case everything is normal.
* If disconnected it should return OPDI_DISCONNECTED.
* Any value that is not OPDI_STATUS_OK will terminate an existing connection.
*/
uint8_t doWork() {
        char key = keypad.getKey();

        if (key != NO_KEY) {
			digitalWrite(13, HIGH);   // set the LED on
			delay(200);              // wait
			digitalWrite(13, LOW);    // set the LED off
			delay(200);              // wait
            if (key >= '0' && key <= '9') {
              // next key entered; multiply previous value
              enteredValue *= 10;
              enteredValue += (key - '0');
            } else {
              // all other keys reset the entered value
              enteredValue = 0;  
            }
            
            if ((codeValue > 0) && (enteredValue == codeValue)) {
              for (int i = 0; i < 10; i++) {
			digitalWrite(13, HIGH);   // set the LED on
			delay(100);              // wait
			digitalWrite(13, LOW);    // set the LED off
			delay(100);              // wait
              }
              enteredValue = 0;  
            }
        }
        
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
		uint8_t result = Opdi->start(&doWork);

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


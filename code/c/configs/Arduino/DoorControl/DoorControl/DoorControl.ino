
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
 * - RTC
 * - RFID access
 * - OPDI slave implementation
 */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <opdi_constants.h>

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

#include <MFRC522.h>    // from: https://github.com/miguelbalboa/rfid
#include <Keypad.h>    // from: http://playground.arduino.cc/code/keypad
#include <DS1307RTC.h>  // from: http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
#include <Time.h>      // from: http://www.pjrc.com/teensy/td_libs_Time.html

#include "ArduinOPDI.h"

#define STATUS_LED    6
#define ACCESS_LED    7

#define RST_PIN         9
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key mifareKey;

// keyboard input state
uint32_t enteredValue = 0;
int64_t codeValue = 0;    // is read from the EEPROM at startup and modified by the dial port

uint32_t timeRefreshCounter;

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
   this->port.extendedInfo = extendedInfo;     
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


// DS1307 dial port: a connection to a DS1307 RTC module
// Gets/sets the current time as an UNIX timestamp (64 bit value, seconds since 1970-01-01)
class OPDI_DS1307DialPort : public OPDI_DialPort {
  
public:
OPDI_DS1307DialPort(const char *id, const char *label, char *extendedInfo) : 
    OPDI_DialPort(id, label, 0, 0x7FFFFFFFFFFFFFFF, 1, 0) {
   this->port.extendedInfo = extendedInfo;     
}

virtual ~OPDI_DS1307DialPort() {}

virtual uint8_t setPosition(int64_t position) {
  tmElements_t tm;

  // convert value (in Unix epoch seconds) to timestamp
  breakTime(position, tm);
  
  if (!RTC.write(tm)) {
    return OPDI_PORT_ERROR;
  }
  
  return OPDI_STATUS_OK;
}

virtual uint8_t getState(int64_t *position) {
  tmElements_t tm;

  if (!RTC.read(tm)) {
    return OPDI_PORT_ERROR;
  }
  // convert time to Unix epoch seconds
  *position = makeTime(tm);
  
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

OPDI_DS1307DialPort rtcPort = OPDI_DS1307DialPort("RTC", "Current time", "unit=keypadCode");

// Keypad definitions
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'7','8','9','E'},
  {'4','5','6','Y'},
  {'1','2','3','D'},
  {'0','.','-','R'}
};
byte rowPins[ROWS] = {A3, A2, A1, A0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4, 5}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

////////////////

uint8_t checkerror(uint8_t result) {
	if (result != OPDI_STATUS_OK) {
		digitalWrite(STATUS_LED, LOW);    // set the LED off
		delay(500);

		// flash error code on LED
		for (uint8_t i = 0; i < result; i++) {
			digitalWrite(STATUS_LED, HIGH);   // set the LED on
			delay(200);              // wait
			digitalWrite(STATUS_LED, LOW);    // set the LED off
			delay(200);              // wait
		}
		digitalWrite(STATUS_LED, LOW);    // set the LED off
		return 0;
	}
	return 1;
}

uint8_t setupDevice() {

  timeRefreshCounter = millis();
  
  tmElements_t tm;
  
	// initialize the digital pin as an output.
	// Pin STATUS_LED has an LED connected on most Arduino boards
	pinMode(STATUS_LED, OUTPUT);

        if (RTC.read(tm)) {
			digitalWrite(STATUS_LED, HIGH);   // set the LED on
			delay(200);              // wait
			digitalWrite(STATUS_LED, LOW);    // set the LED off
			delay(200);              // wait
        }

	// start serial port at 9600 baud
	Serial.begin(9600);

        SPI.begin();        // Init SPI bus
        mfrc522.PCD_Init(); // Init MFRC522 card
    
        // Prepare the key (used both as key A and as key B)
        // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
        for (byte i = 0; i < 6; i++) {
            mifareKey.keyByte[i] = 0xFF;
        }

	// initialize the OPDI system
	uint8_t result = ArduinOpdi.setup("DoorControl", 60000);  // one minute timeout
	if (checkerror(result) == 0)
		return 0;

	// add the ports provided by this configuration
        Opdi->addPort(&eeprom1);
        Opdi->addPort(&rtcPort);
        
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
  
  /*
  // time to refresh the RTC port on a connected master?
  if (millis() - timeRefreshCounter > 3000) {
    if (Opdi->isConnected()) {
      rtcPort.refresh();
    }  
    timeRefreshCounter = millis();
  }
  */
  
        char key = keypad.getKey();

        if (key != NO_KEY) {
			digitalWrite(STATUS_LED, HIGH);   // set the LED on
			delay(200);              // wait
			digitalWrite(STATUS_LED, LOW);    // set the LED off
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
			digitalWrite(STATUS_LED, HIGH);   // set the LED on
			delay(100);              // wait
			digitalWrite(STATUS_LED, LOW);    // set the LED off
			delay(100);              // wait
              }
              enteredValue = 0;  
            }
        }

    // RFID       
            // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return OPDI_STATUS_OK;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return OPDI_STATUS_OK;

    // Show some details of the PICC (that is: the tag/card)
//    Serial.print(F("Card UID:"));
//    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
//    Serial.println();
//    Serial.print(F("PICC type: "));
    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
//    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
//        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return OPDI_STATUS_OK;
    }

    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x08, 0x09, 0xff, 0x0b, //  9, 10, 255, 12,
        0x0c, 0x0d, 0x0e, 0x0f  // 13, 14,  15, 16
    };
    byte trailerBlock   = 7;
    byte status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
//    Serial.println(F("Authenticating using key A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &mifareKey, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
//        Serial.print(F("PCD_Authenticate() failed: "));
//        Serial.println(mfrc522.GetStatusCodeName(status));
        return OPDI_STATUS_OK;
   }
        
	return OPDI_STATUS_OK;
}

void loop() {

	// do housekeeping
	doWork();

	// character received on serial port? may be a connection attempt
	if (Serial.available() > 0) {

		// indicate connected status
		digitalWrite(STATUS_LED, HIGH);   // set the LED on

		// start the OPDI protocol, passing a pointer to the housekeeping function
		// this call blocks until the slave is disconnected
		uint8_t result = Opdi->start(&doWork);

		// no regular disconnect?
		if (result != OPDI_DISCONNECTED) {
			digitalWrite(STATUS_LED, LOW);    // set the LED off
			delay(500);

			// flash error code on LED
			for (uint8_t i = 0; i < result; i++) {
				digitalWrite(STATUS_LED, HIGH);   // set the LED on
				delay(200);              // wait
				digitalWrite(STATUS_LED, LOW);    // set the LED off
				delay(200);              // wait
			}
		}

		digitalWrite(STATUS_LED, LOW);    // set the LED off
	}
}


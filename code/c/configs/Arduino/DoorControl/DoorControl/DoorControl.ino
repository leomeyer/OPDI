//    Door Control
//
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
#define RELAY         8

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Special port definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////

// EEPROM dial port: a 10 byte EEPROM sequence starting at the specified address
// The first 8 bytes are the 64 bit value (LSB first). The last two bytes are the checksum.
// If the checksum is not valid the position returned will be 0.
class OPDI_EEPROMDialPort : public OPDI_DialPort {
protected:
  int address;
  
public:
OPDI_EEPROMDialPort(const char *id, const char *label, const int address, const int64_t minValue, const int64_t maxValue, char *extendedInfo) : 
    OPDI_DialPort(id, label, minValue, maxValue, 1, 0) {
   this->address = address;
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
// Technically the value is in the UTC time zone but we treat it as local time.
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

/** Defines a digital port that opens the door using the specified relay pin.
 *
 */
class OPDI_DigitalPortDoor : public OPDI_DigitalPort {

public:
OPDI_DigitalPortDoor(const char *id, const char *name) : OPDI_DigitalPort(id, name, OPDI_PORTDIRCAP_OUTPUT, 0) {  
}

uint8_t setMode(uint8_t mode) {
  return OPDI_STATUS_OK;
}

uint8_t setLine(uint8_t line) {
  if (line == 1) {
    // set relay port to low
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);
    delay(2000);              // wait
    pinMode(RELAY, INPUT);
  }  
  return OPDI_STATUS_OK;
}

uint8_t getState(uint8_t *mode, uint8_t *line) {
  *mode = OPDI_DIGITAL_MODE_OUTPUT;
  *line = 0;
  return OPDI_STATUS_OK;
}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RST_PIN         9
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// keyboard input state
uint32_t enteredValue = 0;
// for periodic time refresh of a connected master
uint32_t timeRefreshCounter;

// The ArduinOPDI class defines all the Arduino specific stuff like serial ports communication.
ArduinOPDI ArduinOpdi = ArduinOPDI();
// define global OPDI instance
// important: for polymorphism to work, assign own OPDI instance
OPDI* Opdi = &ArduinOpdi;

// Port definitions

OPDI_EEPROMDialPort codePort = OPDI_EEPROMDialPort("EDP1", "Code", 0, 0, 99999, "unit=keypadCode");

OPDI_DS1307DialPort rtcPort = OPDI_DS1307DialPort("RTC", "Current time", "unit=unixTime");

OPDI_DigitalPortDoor doorPort = OPDI_DigitalPortDoor("DOOR", "Door");

OPDI_EEPROMDialPort tag1Port = OPDI_EEPROMDialPort("TAG1", "Tag 1", 10, 0, 999999999999, "");
//OPDI_EEPROMDialPort tag2Port = OPDI_EEPROMDialPort("TAG2", "Tag 2", 2, 0, 2 << 32, "unit=keypadCode");
//OPDI_EEPROMDialPort tag3Port = OPDI_EEPROMDialPort("TAG3", "Tag 3", 3, 0, 2 << 32, "unit=keypadCode");
OPDI_EEPROMDialPort lastTagPort = OPDI_EEPROMDialPort("LTAG", "Last Tag", 20, 0, 999999999999, "");
//OPDI_EEPROMDialPort lastAccessPort = OPDI_EEPROMDialPort("LAC", "Last Access", 4, 0, 2 << 32, "unit=unixTime");

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

// time data structure
tmElements_t tm;

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
////////////////////////////////////////////////////////////////////////////////////////////////////////
// Routines
////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

uint8_t setupDevice() {
  // initialize the digital pin as an output.
  // Pin STATUS_LED has an LED connected on most Arduino boards
  pinMode(STATUS_LED, OUTPUT);

  // initialize the OPDI system
  uint8_t result = ArduinOpdi.setup("DoorControl", 60000);  // one minute timeout
  if (checkerror(result) == 0)
    return 0;

  // read/configure the RTC
  if (!RTC.read(tm)) {
    // not able to read; maybe the time was not set
    // get the date and time the compiler was run
    if (getDate(__DATE__) && getTime(__TIME__)) {
      // and configure the RTC with this info
      RTC.write(tm);
    }
  }
  
  // try to read the RTC
  if (RTC.read(tm)) {
    // RTC ok, add the clock port
    Opdi->addPort(&rtcPort);
  }

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  mfrc522.PCD_SetAntennaGain((0x07<<4));

  // add the ports provided by this configuration
  Opdi->addPort(&codePort);
  Opdi->addPort(&doorPort);
  Opdi->addPort(&tag1Port);
  //Opdi->addPort(&tag2Port);
  //Opdi->addPort(&tag3Port);
  Opdi->addPort(&lastTagPort);
//  Opdi->addPort(&lastAccessPort);

  // start serial port at 9600 baud
  Serial.begin(9600);

  timeRefreshCounter = millis();

  return 1;
}

void openDoor() {

  /*
  for (int i = 0; i < 10; i++) {
    digitalWrite(STATUS_LED, HIGH);   // set the LED on
    delay(100);              // wait
    digitalWrite(STATUS_LED, LOW);    // set the LED off
    delay(100);              // wait
  }
  */
  
  doorPort.setLine(1);
/*
  if (RTC.read(tm)) {
     int64_t accessTime = makeTime(tm);
     // store last time in EEPROM
     lastAccessPort.setPosition(accessTime);
  }  
*/
}

/* This function can be called to perform regular housekeeping.
* Passing it to the Opdi->start() function ensures that it is called regularly.
* The function can send OPDI messages to the master or even disconnect a connection.
* It should always return OPDI_STATUS_OK in case everything is normal.
* If disconnected it should return OPDI_DISCONNECTED.
* Any value that is not OPDI_STATUS_OK will terminate an existing connection.
*/
uint8_t doWork() {
  // time to refresh the RTC port on a connected master?
  if (millis() - timeRefreshCounter > 3000) {
    if (Opdi->isConnected()) {
      rtcPort.refresh();
    }  
    timeRefreshCounter = millis();
  }
  
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
    
    int64_t codeValue;
    // read keypad code value from EEPROM
    codePort.getState(&codeValue);

    if ((codeValue > 0) && (enteredValue == codeValue)) {
      // success
      openDoor();
      enteredValue = 0;  
    }
  }

  // RFID       
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
    return OPDI_STATUS_OK;

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
    return OPDI_STATUS_OK;

  // valid tag ID found?
  if (mfrc522.uid.size == 4) {
     uint32_t uid = *(uint32_t*)mfrc522.uid.uidByte;
     
    // store last tag ID in EEPROM
    lastTagPort.setPosition(uid);
  
    // check against stored tag IDs
    bool success = false;
    int64_t tagID;
    tag1Port.getState(&tagID);
    success = tagID == uid;
    /*
    if (!success) {
      tag2Port.getState(&tagID);
      success = tagID == uid;
    }
    if (!success) {
      tag3Port.getState(&tagID);
      success = tagID == uid;
    }
    */
    if (success) {
      openDoor();
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

int main(void)
{
  init();

  uint8_t setupOK = setupDevice();

  for (;;)
    if (setupOK)
      loop();

  return 0;
}



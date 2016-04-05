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

// CAUTION! This is about the maximum of RAM that can be used without stability problems!
// Be careful with modifications, especially string constants!

/** DoorControl for Arduino
 * - Keypad
 * - RTC
 * - RFID access (optional)
 * - OPDI slave implementation
 *
 * Install the provided libraries by copying them to your Arduino libraries folder.
 * Copy the OPDI files to this folder (you can use the provided batch file for this)
 * and restart the Arduino IDE, then compile.
 */

// Uncomment this define if you want to use an RFID receiver (MFRC522 is supported).
// #define USE_RFID

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

#include <Keypad.h>    // from: http://playground.arduino.cc/code/keypad
#include <DS1307RTC.h>  // from: http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
#include <Time.h>      // from: http://www.pjrc.com/teensy/td_libs_Time.html

#ifdef USE_RFID
#include <MFRC522.h>    // from: https://github.com/miguelbalboa/rfid
#endif

#include "opdi_constants.h"

#include "DoorControl_secrets.h"

#include "ArduinOPDI.h"

#define STATUS_LED    6
#define SWITCH        7
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
  char *extendedInfo;
public:
OPDI_EEPROMDialPort(const char *id, const char *label, const int address, const int64_t minValue, const int64_t maxValue, char *extendedInfo) : 
    OPDI_DialPort(id, label, minValue, maxValue, 1, 0) {
   this->address = address;
   this->extendedInfo = extendedInfo;     
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

virtual uint8_t getExtendedInfo(char *buffer, size_t length) {
  strncpy(buffer, this->extendedInfo, length);
  return OPDI_STATUS_OK;
}
};

// DS1307 dial port: a connection to a DS1307 RTC module
// Gets/sets the current time as an UNIX timestamp (64 bit value, seconds since 1970-01-01)
// Technically the value is in the UTC time zone but we treat it as local time.
class OPDI_DS1307DialPort : public OPDI_DialPort {
public:
OPDI_DS1307DialPort(const char *id, const char *label) : 
    OPDI_DialPort(id, label, 0, 999999999999, 1, 0) {
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

virtual uint8_t getExtendedInfo(char *buffer, size_t length) {
  strncpy(buffer, "unit=unixTime", length);
  return OPDI_STATUS_OK;
}
};

/** Defines a digital port that opens the door using the specified RELAY pin.
 * The SWITCH pin is read to indicate whether the door is open or closed.
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
  if (digitalRead(SWITCH))
    *line = 0;
  else
    *line = 1;
  return OPDI_STATUS_OK;
}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_RFID
#define RST_PIN         9
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
#endif

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
OPDI_EEPROMDialPort codePort = OPDI_EEPROMDialPort("C", "Code", 0, 0, 99999, "unit=keypadCode");
OPDI_DS1307DialPort rtcPort = OPDI_DS1307DialPort("T", "Time");
OPDI_DigitalPortDoor doorPort = OPDI_DigitalPortDoor("D", "Door");
OPDI_EEPROMDialPort lastAccessPort = OPDI_EEPROMDialPort("A", "LAcc", 50, 0, 999999999999, "unit=unixTime"); // store time for keypad opening only
#ifdef USE_RFID
OPDI_EEPROMDialPort tag1Port = OPDI_EEPROMDialPort("1", "Tag1", 10, 0, 999999999999, "");
OPDI_EEPROMDialPort tag2Port = OPDI_EEPROMDialPort("2", "Tag2", 20, 0, 999999999999, "");
OPDI_EEPROMDialPort lastTagPort = OPDI_EEPROMDialPort("L", "LTag", 40, 0, 999999999999, "");
#endif

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

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// time data structure
tmElements_t tm;

// month name constants for compile timestamp parsing
const char str_jan[] PROGMEM = "Jan";
const char str_feb[] PROGMEM = "Feb";
const char str_mar[] PROGMEM = "Mar";
const char str_apr[] PROGMEM = "Apr";
const char str_may[] PROGMEM = "May";
const char str_jun[] PROGMEM = "Jun";
const char str_jul[] PROGMEM = "Jul";
const char str_aug[] PROGMEM = "Aug";
const char str_sep[] PROGMEM = "Sep";
const char str_oct[] PROGMEM = "Oct";
const char str_nov[] PROGMEM = "Nov";
const char str_dec[] PROGMEM = "Dec";
const char* const monthName[12] PROGMEM = {
  str_jan, str_feb, str_mar, str_apr, str_may, str_jun,
  str_jul, str_aug, str_sep, str_oct, str_nov, str_dec
};

// last door state for refresh of door port
uint8_t lastDoorState = -1;

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

// date and time parsing from compiler timestamp
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
  char monthBuf[4];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    strcpy_P(monthBuf, (char*)pgm_read_word(&(monthName[monthIndex])));
    if (strcmp(Month, monthBuf) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

uint8_t setupDevice() {
  // LED pin as output
  pinMode(STATUS_LED, OUTPUT);
  
  // switch as input, pullup on
  pinMode(SWITCH, INPUT);
  digitalWrite(SWITCH, HIGH);

  // initialize the OPDI system
  uint8_t result = ArduinOpdi.setup(60000, OPDI_FLAG_AUTHENTICATION_REQUIRED, &Serial, NULL);  // one minute timeout
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

  // add the ports provided by this configuration
  Opdi->addPort(&codePort);
  Opdi->addPort(&doorPort);
  Opdi->addPort(&lastAccessPort);

#ifdef USE_RFID
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  mfrc522.PCD_SetAntennaGain((0x07<<4));
  Opdi->addPort(&tag1Port);
  Opdi->addPort(&tag2Port);
  Opdi->addPort(&lastTagPort);
#endif

  // start serial port at 9600 baud
  Serial.begin(9600);

  timeRefreshCounter = millis();

  return 1;
}

void openDoor() {
  digitalWrite(STATUS_LED, HIGH);   // set the LED on
  doorPort.setLine(1);
  digitalWrite(STATUS_LED, LOW);    // set the LED off
}

/* This function performs regular housekeeping.
* It is used to check whether the door relay should be activated.
* First it checks the keypad and determines the total entered code.
* If the code matches the one stored in the EEPROM the door is opened and
* the last access time is stored in the EEPROM.
* Next, if an RFID tag is detected its ID is checked against the three
* tag IDs stored in the EEPROM. If one of them matches the door is opened.
*/
uint8_t doWork() {
  // time to refresh the RTC port on a connected master?
  if (millis() - timeRefreshCounter > 3000) {  // every three seconds
    if (Opdi->isConnected()) {
      rtcPort.refresh();
    }  
    timeRefreshCounter = millis();
  }
  
  // door state changed?
  uint8_t doorState = digitalRead(SWITCH);
  if (doorState != lastDoorState)
    if (Opdi->isConnected()) {
      doorPort.refresh();
    }  
  lastDoorState = doorState;
  
  // check keypad
  char key = keypad.getKey();

  if (key != NO_KEY) {
    digitalWrite(STATUS_LED, HIGH);   // set the LED on
    delay(100);              // wait
    digitalWrite(STATUS_LED, LOW);    // set the LED off
    delay(100);              // wait
    if (key >= '0' && key <= '9') {
      // next key entered; multiply previous value
      enteredValue *= 10;
      enteredValue += (key - '0');
    } else {
      // all other keys reset the entered value
      enteredValue = 0;  
    }
    
    // during daytime (8:00 - 18:00) the regular keycode opens the door
    // also, if the RTC is not working
    if (!RTC.read(tm) || ((tm.Hour >= 8) && (tm.Hour < 18))) {
      int64_t codeValue;
      // read keypad code value from EEPROM
      codePort.getState(&codeValue);
  
      if ((codeValue > 0) && (enteredValue == codeValue)) {
        // success
        openDoor();
        enteredValue = 0;
        // store last access time if possible
        if (RTC.read(tm)) {
           int64_t accessTime = makeTime(tm);
           // store last time in EEPROM
           lastAccessPort.setPosition(accessTime);
        }  
      }
    }
    
    // the special code always opens the door
    if (enteredValue == SPECIAL_DOORCODE) {
      openDoor();
      enteredValue = 0;
    }
  }

  // check open door switch (switch on status LED if closed)
  if (digitalRead(SWITCH) == 0) {
    digitalWrite(STATUS_LED, HIGH);   // set the LED on
  } else {
    digitalWrite(STATUS_LED, LOW);   // set the LED off
  }

#ifdef USE_RFID
  // check RFID tags
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
    if (!success) {
      tag2Port.getState(&tagID);
      success = tagID == uid;
    }
    if (success) {
      openDoor();
    }
  }
#endif
  
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
        delay(100);              // wait
        digitalWrite(STATUS_LED, LOW);    // set the LED off
        delay(100);              // wait
      }
    }
    
    digitalWrite(STATUS_LED, LOW);    // set the LED off
  }
}

int main(void)
{
  init();
  
  // let everything stabilize (needed for RTC?)
  delay(500);

  uint8_t setupOK = setupDevice();

  for (;;)
    if (setupOK)
      loop();

  return 0;
}












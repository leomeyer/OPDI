OPDI
====

Open Protocol for Device Interaction

*Please note: Code in this repository is under active development*

Welcome to the OPDI project. The Open Protocol for Device Interaction defines
a communication protocol that can be used to interact with a variety of devices
from small microcontrollers up to larger systems. It defines a few key concepts
that simplify and standardize device functions in order to be able to control
them via generalized UI elements. The protocol is designed to be lightweight,
extensible and reasonably secure.

This repository provides a number of reference and example implementations as
well as related projects. The most interesting project is probably OPDID, a 
home automation server that is based on key concepts and implementations
of the OPDI protocol. Please see:
https://github.com/leomeyer/OPDI/blob/master/code/c/configs/opdid/doc/index.md

OPDID also provides an HTML GUI and a JSON-RPC API. Example screenshot:

![OPDID screenshot](doc/images/OPDID_screenshot_1.png)

An OPDI server is called a "slave". There are currently implementations for
slaves on Windows, Linux (including Raspberry Pi), the Arduino IDE as well as
Atmel 8 bit microcontrollers (using WinAVR) with at least 16 kB of flash ROM.

The software used to control an OPDI slave is called a "master". Currently there
is a feature-complete implementation of an Android master that can be used to
control slaves via Bluetooth or TCP/IP. There is also an incomplete C++ master.

Example screenshot of the Android master:

![AndroPDI screenshot](doc/images/AndroPDI_screenshot_1.png)

Please browse the repository for more information. The general structure is
as follows:

	root
		code
			java						- contains the JAVA master implementation
				common					- JAVA OPDI library
				configs
					AndroPDI			- Android master control program
			c							- contains C and C++ implementations
				common					- OPDI base library (C, and C++ wrappers)
				configs					- actual slave and master implementations
					Arduino
						DoorControl		- example implementation: Bluetooth-controlled door opener
					ArduinoPDI			- Eclipse AVR/C++ project for Arduinos
					Chromashade			- Atmega16 4-channel LED PWM project
					LinOPDI				- test implementation for Linux
					Poladisc			- Atmega16 4-channel LED PWM with stepper motor control
					RaspOPDI			- test implementation for Raspberry Pi
					test				- common test code
					WinOPDI				- test implementation for Windows
					opdid				- root folder for the OPDID home automation server
						opdid			- OPDID main executable folder
						plugins			- plugin extensions for OPDID
						doc				- OPDID documentation
				libraries				- support libraries for configs (mainly for OPDID)
				platforms				- platform specific implementations
		doc								- OPDI documentation

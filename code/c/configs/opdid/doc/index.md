
##OPDID Automation Software

OPDID, the Open Protocol for Device Interaction Daemon, is an open-source, cross-platform automation server written in C++11 for performance and small memory footprint. It can be configured for a variety of automation tasks, for example home automation.
It has the following features:


 - Written in C++11 for performance and small memory footprint
 - Ideally suited for Raspberry Pis and other Linux based systems
 - Configurable by text files in INI file format
 - Controllable via an Android app or HTML/Javascript (with a JSON-RPC web service API)
 - Supports a plugin architecture for extensibility
 - Controls a variety of actors, for example relays or dimmers
 - Accepts input from switches or A/D converters
 - Supports scene files for predefined combinations of settings
 - Stores user settings in persistent files
 - Has plugins for:
	 - Raspberry Pi Gertboard I/O board utilizing the onboard ATMEGA as a port expander
	 - Weather station support (interoperates with the Weewx weather station software)
	 - Window and shutter control with a variety of options
	 - Fritz!Box smart home devices
	 - An integrated web server
	 - Radio-controlled power sockets
 - Built-in functions:
	 - Timer that supports periodical, astronomic (sunset/sunrise) or manual scheduling
	 - Expression engine for complex mathematical calculations
	 - Interoperation with the OS: Monitor files to read input, execute OS commands
	 - Logic functions for changing outputs based on logical conditions
	 - Pulse with configurable duration and duty cycle
	 - Fader for smooth control of e. g. dimmers
	 - Statistical aggregator for calculations of means etc.
	 - Trigger that is activated when inputs change
	 -  and more

OPDID allows you to monitor and control the state of sensors and actors such as lamps, fans, dimmers, windows and shutters, radio-controlled power sockets, weather sensors, electric, gas and water meters and many more.

Typical use cases in home automation include:

 - Open or close windows and shutters depending on sunrise or sunset
 - Ensure that windows stay closed when it rains or if it is too cold outside
 - Switch on the lights or fade in a dimmer in the morning
 - Display current meter values and water or energy consumption statistics
 - Show current weather conditions
 - Control heating equipment  
 
Basic Concepts
==============

In order to work with OPDID it is necessary to understand some basic concepts. This will also give you some understanding of OPDID's flexibility and limitations.

The basic automation building block is called a "Port". Everything in OPDID revolves around the configuration of ports and their connections. Ports encapsulate simple or advanced behavior and get/provide information from/to other ports. It is the combination of ports that makes the emergence of complex automation behavior possible.

You can think of ports as internal variables of the system. Ports can also be exposed on a user interface, giving read-only or writable access to the user. OPDID can automatically log the values of ports. At the same time, ports contain the system's functionality like window operation state machines etc.

There are different types of ports in OPDID. To be able to model automation behavior with ports you'll have to learn about these different types of ports and their properties and functions. The basic properties of all types of ports are:

	- A unique ID which is a text string preferably without blanks and special characters, for example "Window1"
	- A "Hidden" flag which decides whether to display this port on a user interface or not
	- An optional label and icon specification to display on a user interface (if the port is not hidden)
	- An optional unit specification which determines value conversions, formatting etc.
	- A "Readonly" flag which decides whether a user may change this port's value
	- A "Persistent" flag which decides whether the state of this port is remembered permanently
	- An optional group ID (ports can be ordered into hierarchical groups)
	- A refresh mode that decides how state changes affect the connected user interfaces
	- Some port specific flags and capabilities settings
	- A freely assignable tag value

Basically, ports are of four different types:

Digital Port
------------

A Digital port is the most elementary port type. A Digital port has a "Mode" and a "Line" state. Modes can be input and output, and the line can be either Low or High.

The Digital port is modeled after a digital I/O pin of a microcontroller. The important thing here is that a Digital port's state is always known, and it is either Low or High. Essentially, the Digital port models a switch. In OPDID it is used for "things that can be on or off", no matter whether the state of these things is controlled directly by a user or by internal functions, or whether the Digital port logically reflects the state of some outside sensor (by reading the state from some kind of driver or whatever).

Typical examples of Digital ports are LEDs or relays (output only), and switches or buttons (input only). However, most higher-level functions in OPDID are also modeled as Digital ports, thus giving you the ability to switch these functions on or off. For example, a Pulse port can periodically change the state of one or more Digital ports ("outputs"). The Pulse port itself is again a Digital port, and it will only be active if its Line state is High. Thus, you can switch the Pulse port on or off using a user interface, or the Pulse port can again be controlled by any other port that accepts a Digital port as an output. In this way you can connect elementary blocks together to form complex behavior.


Getting started
===============


Miscellaneous information
=========================

Logging
-------

* Log levels
* Individual ports can override the global log level

Heartbeat file
--------------



User account settings
---------------------

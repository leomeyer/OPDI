
OPDID Automation Server Overview
================================

**OPDID** stands for "Open Protocol for Device Interaction Daemon". The OPDI protocol is a way of remote-controlling devices. 
OPDID extends this functionality by implementing automation functions. As such, it can be used for, e. g.:

* home automation
* data logging
* interactive art projects
* and many more. 

OPDID supports manual as well as automatic control of actors like lamps, dimmers, windows, shutters etc.
It also can read data from sensors like switches, analog inputs, weather stations, and others.

OPDID is written in mostly platform independent C and C++.
Its versatile standard functions allow you to get started quickly with your automation project.
It also integrates with operating system tools to read data from or interact with other processes.
For advances uses it features a plugin mechanism that allows you to write your own extensions to interface with custom devices.

OPDID is meant to run on hardware like the Raspberry Pi. To interface with your sensors and actors you will most likely
need additional interface electronics like buffers, relays, or A/D converters.

Basic concepts
--------------

When a remote control application connects to OPDID (using the OPDI protocol), OPDID acts as a **slave**. The connecting app
is called the **master**.

Actors and sensors are internally represented as **ports**. Ports are the basic building blocks of any automation project.
They have specific representations on the user interface of connected masters. Internally you can think of ports like variables.
Each port always has a well-defined state, depending on its type. A port can also be in an error state (for example, if a
sensor cannot be read).

Ports are of different types; each port type has specific capabilities and limitations. OPDID supports the following port types:

Digital port
~~~~~~~~~~~~

.. image:: images/digital_port.png

A digital port has only two states: **Low** and **High**. It is used for devices whose state is always known: for example, lamps
or switches (which can be either on or off). If the state is not always known, for example if the device is output only (like
certain radio controlled power sockets that do not support a status channel) a digital port is not used.

A digital port operates in either **Input**, **Output**, or **Bidirectional** mode. If you want to capture the input of a switch you could
use a digital port in Input mode. If you want to control a relay you should use a digital port in Output mode.
In rare occasions you might specify a digital port to operate in Bidirectional mode; in this case the user can select whether
the port is input or output.

Some devices might support different input modes with respect to connected resistors. OPDID digital ports support three different input modes:
**Floating**, **Input with pullup**, and **Input with pulldown** resistor. In most cases, though, the hardware wiring will already
define the resistor configuration in which case this distinction is less important.


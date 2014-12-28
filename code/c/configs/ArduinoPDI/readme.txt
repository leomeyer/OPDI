The ArduinOPDI project is a part of the OPDI reference implementation.

It implements an OPDI slave for Arduinos, allowing you to control Arduino IO ports using a smartphone.

To set up Arduino development for Eclipse, see here: http://forum.arduino.cc/index.php/topic,37788.0.html

Roughly, it works as follows:
1. Install Eclipse
2. Install Eclipse C++ Development Tools
3. Install the Eclipse AVR plugin
4. Install the AVG-GCC toolchain (it's best if you use the toolchain that comes with the Arduino IDE).

This project can be used to program Arduinos directly. There is also an Ant build file that creates an arduino library
for import into the regular Arduino IDE. This library can then serve as base for own projects.

To switch between the configurations for different Arduino models do the following:

1. Open the Project Properties.
2. Go to C/C++ Build -> Build Variables.
3. For the Variable ARDUINO_TARGET, specify the desired value. This value must correspond with a folder name in the lib directory.
4. Go to AVR -> Target Hardware. Select the CPU model for the Arduino model.
5. If you are using the programmer from Eclipse, go to AVR -> AVRDude. Select the programmer to be used for the Arduino model.
6. From your model's lib directory, delete the file libarduinopdi.a to make sure that it gets rebuilt.

Clean and recompile the project. If you are connecting to the Arduino via a Bluetooth module on the serial lines TX and RX it may be necessary
to temporarily disconnect these wires while flashing over USB. 

Window automation plugin for OPDID

This plugin provides the functionality to operate motor-controlled windows.
A window can be either fully closed or fully open. Intermediate states are not supported.
The plugin provides a SelectPort that has at least two items for the closed and open state.
You must provide the usual [<SelectPort-ID>.Items] section for the labels of the states. The first
of these items corresponds with the closed state of the window. The second item corresponds with
the open state of the window. All other items disable the manual control of the window and switch
it to automatic control.

The window supports an optional sensor to detect the closed position. You can specify a DigitalPort
node for the plugin to treat as a sensor input.

The motor is controlled by two lines MotorA and MotorB whose truth table is as follows:

MotorA | MotorB | Motor action
-------+--------+-----------------------
     0 |      0 | No movement
     1 |      0 | Turn in "open" direction
     0 |      1 | Turn in "close" direction
   (1) |    (1) | Not used

You can specify that the line levels should be inverted by setting MotorActive to 0.

Optionally, there is a third line called Enable which can be activated before the motor action
should take place. This is useful e. g. if there is a separate power supply for the motor which should not
be always active. The delay between enabling/disabling and the motor action can be set using EnableDelay.
You can specify that the line level should be inverted by setting EnableActive to 0.

The lines A and B and (optionally) Enable must be connected to DigitalPort nodes (in Output mode).

As there is no end position sensor for the fully open position you have to experimentally determine
a setting for the time the window needs to open completely. This time in milliseconds must be specified in the
OpeningTime setting. If you are using no closing sensor you should also set the ClosingTime setting.
If this setting is not specified it is set to the value of OpeningTime.

After detecting a closed sensor the motor can remain active for some time to ensure that the window is fully closed.
Use the MotorDelay setting to specify this time in milliseconds. This value must be lower or equal to EnableDelay.

The select port gives the user the choice about the window state. In the automatic control setting,
the window state is controlled by the settings AutoOpen and AutoClose. You can specify any number of
DigitalPort IDs in these settings. If any of these ports has a line state of High this will trigger opening 
or closing the window respectively. The AutoClose setting takes precedence.
AutoOpen and AutoClose will only work in the automatic mode. In manual Closed and Open modes changes of
these lines have no effect.

Additionally you can specify one or more DigitalPorts that force the window to the closed state 
if at least one port has a line state of High. This mechanism can be used to close the window (e. g. in 
case there is rain etc). This mechanism also works when the mode has been set to "manual open". In this case,
if a forced close happens the position of the select port will be set to Closed.
As long as one of the ForceClose ports is High, the window cannot be opened any more.
There is currently no way to force a window to the open state.

The window can detect some errors that may arise due to hardware failures or misconfigurations.
An error occurs if 
a) the window is opened and after the specified opening time the sensor still reads "Closed",
b) the window is closed and after the specified closing time the sensor does not read "Closed".

The error can be due to
- sensor failure (switch misaligned, contact problem, cable interruption...)
- motor failure (power failure, contact problem, motor damage...)
- blocked window (doesn't fully close any more due to obstructions...)
- misconfiguration (wrong sensor or motor ports specified)
- etc.

The error will be shown on the master. In case of error there is no meaningful positional setting of the
select port any more. Also, automatic control, including the ForceClose mechanism, will be disabled.
The window will remain in the error state until the slave restarts or the select port position is changed
to Closed or Open. This serves as a protection in case of some serious problem. 

If the position is manually opened or closed after an error the window will try to put itself into the specified
position. If this works the mode can again be set to automatic. Otherwise the window will return to the error state.

Example section:

[Window1]
Driver = window.dll
; This setting is required.
Type = SelectPort
Label = Window1
; Hide the port if you want automatic control only.
Hidden = false
; Set initial position; 0: closed; 1: open; 2: automatic control
Position = 2

; Specifies the DigitalPort input sensor.
Sensor = Window1ClosedSwitch

; Specifies when the window is closed. 0 or 1. Defaults to 1.
SensorClosed = 1

; Specifies the DigitalPort output lines that act on the motor driver (e. g. an H-bridge).
MotorA = Window1A
MotorB = Window1B

; Specifies when the motor is active. 0 or 1. Defaults to 1.
MotorActive = 1

; Specifies the time to keep the motor running when closing after detecting the close sensor (milliseconds).
; Has no effect if no close sensor is specified. Must be lower or equal to EnableDelay.
MotorDelay = 1000

; Specifies the DigitalPort output line that enables the motor driver.
Enable = WindowMotor24V

; Specifies when the motor is active. 0 or 1. Defaults to 1.
EnableActive = 1

; Specifies the time to wait before motor action after enabling and before disabling after motor action (milliseconds).
EnableDelay = 2000

; Specifies the time to keep the motor running until the window fully opens (milliseconds).
; It takes 10 seconds to fully open the window in this example.
; After turning the motor for this time the motor is stopped and EnableDelay starts to count.
; If no other command is issued during this time Enable is set to Low.
OpeningTime = 10000

; Specifies the time to keep the motor running until the window fully closes (milliseconds).
; If the optional sensor indicates before this time the motor is stopped. If there is no sensor, 
; after turning the motor for this time the motor is stopped and EnableDelay starts to count.
; If no other command is issued during this time Enable is set to Low.
ClosingTime = 12000

; AutoOpen and AutoClose take an arbitrary number of DigitalPorts. In automatic mode, if one of them is High 
; the window will try to switch to the specified mode.
AutoOpen = 
AutoClose =

; If one of the DigitalPorts specified here is High the window will close. It cannot be opened until all
; ports are Low again.
ForceClose = RainSensor

[Window1.Items]
Window 1 closed = 1
Window 1 open = 2
Window 1 automatic = 3

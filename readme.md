# CME 495 Capstone Project
## Anemoi Design Group
###### Boreas Control Platform on behalf of MST Global

The Boreas Control Platform (BCM) is an automated system designed to remotely control fans and doors within an underground mining operating.

This software includes rpi files (Raspberry Pi) and tm4c files (tm4c123gh6pm)
The tm4c implements the control that turns on fans and opens/closes doors.
The rpi files act as a SCADA system for the BCM. It collects rule changes from
the Impact Communications Appliance, sends requests to the tm4c, and waits for
acknowledgements of error free transmission.

Full operation of this software requires MST Wireless Access points, Aeroscout RFID tags, a network switch, and the Impact Communications Appliance.

The code base uses [libmodbus](https://github.com/stephane/libmodbus.git)


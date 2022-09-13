OpenKayo Kayo-A4 GCode Adapter
==============================

Connects to the serial interface header of a Kayo-A4 pick-and-place
machine, and provides a USB interface with a GCode interpreter that
translates messages into the format necessary for the factory Kayo
firmware.

The hardware portion of the project can minimally be built with an
ESP32-based dev board such as a Wemos D1 Mini and some cables.

A dedicated hardware design is also included, which provides additional
features such as a CAN bus interface to integrate with other devices
on the production line.

More information is available at:

  https://www.superhouse.tv/


Hardware
--------
The "Hardware" directory contains the PCB design as an EAGLE project.


Firmware
--------
The "Firmware" directory contains example firmware as an Arduino
project.


Enclosure
---------
The "Enclosure" directory contains STLs for a case that can be
3D-printed.


Credits
-------
 * Jonathan Oxer <jon@oxer.com.au>


License
-------
Copyright 2022 SuperHouse Automation Pty Ltd  www.superhouse.tv  

The hardware portion of this project is licensed under the TAPR Open
Hardware License (www.tapr.org/OHL). The "license" folder within this
repository contains a copy of this license in plain text format.

The software portion of this project is licensed under the Simplified
BSD License. The "licence" folder within this project contains a
copy of this license in plain text format.


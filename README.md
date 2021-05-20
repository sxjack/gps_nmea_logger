# GPS NMEA Logger

An Arduino program that logs NMEA data from a GPS to an SD card.

Intended for use with OpenLog hardware. Developed using an Arduino Zero clone.

If the GPS takes Ublox commands and is on 9600 (eg a BN-220 out of the packet),
it will be configured on startup for 3D airborne and the baud and update rates
set in the program.

The program will use either the standard Arduino SD library or
Greiman's SdFat (https://github.com/greiman/SdFat).

When programming OpenLog units, use a 3.3V FTDI/programmer.

Hardware

* OpenLog
* BN-220
* FTDI USB/serial interface

The standard OpenLog firmware is available at https://github.com/sparkfun/OpenLog. 
A diagram showing how to connect an OpenLog to an FTDI is in the documentation
directory.
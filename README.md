# GPS NMEA Logger

An Arduino program that logs NMEA data from a GPS to an SD card.

Intended for use with OpenLog hardware. Also tested on an Arduino Zero clone.

If the GPS takes Ublox commands and is on 9600 (eg a BN-220 out of the packet),
it will be configured on startup for 3D airborne and the baud and update rates
set in the program.

The program will use either the standard Arduino SD library or
Greiman's SdFat (https://github.com/greiman/SdFat).

Hardware

* OpenLog
* BN-220
* FTDI USB/serial interface



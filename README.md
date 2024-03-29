# GPS NMEA Logger

An Arduino program that logs NMEA data from a GPS to an SD card.

Intended for use with OpenLog hardware.
Developed using an STM32F1 'blue pill'.

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

This program should be easy to port to any other Arduino with an SD module.

The standard OpenLog firmware is available at https://github.com/sparkfun/OpenLog. 
A diagram showing how to connect an OpenLog to an FTDI is in the documentation
directory.

![OpenLog and BN-220](images/logger.jpg)

![Logged data converted to gpx and displayed in Google Earth](images/gps_log.jpg)

`nmea_logger.sparkfun_openlog.hex` is compiled for the SparkFun OpenLog. The command to upload 
it will be something like `C:\Users\xxxx\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17/bin/avrdude -CC:\Users\xxxx\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17/etc/avrdude.conf -v -patmega328p -carduino -PCOM44 -b115200 -D -Uflash:w:nmea_logger.sparkfun_openlog.hex:i` but the path and com port will need changing.

minixie
=======
![minixie](https://raw.githubusercontent.com/wbober/minixie/master/doc/minixie.jpg)

This project is my take on creating a simple Nixie tube clock. The design uses few
components but packs some nice features.

Features
--------
* a real time clock with super cap backup
* a buzzer
* a light intensity sensor
* an auxiliary connector (UART, IRQ, GPIO, +5V)
* support for an optional DCF77 decoder

Hardware
--------
* only 3 ICs (7805, ATMega8, and 74141 driver)
* single side PCBs
* through-hole components
* designed for LC531 tubes
* small size (46x108mm)
* EagleCAD source for schematics and PCB

Software
--------
* written in C
* tested with AVRStudio/Eclipse

License
-------
Copyright (c) 2012-2014 Wojciech Bober
Licensed under the GNU GPL v2 license
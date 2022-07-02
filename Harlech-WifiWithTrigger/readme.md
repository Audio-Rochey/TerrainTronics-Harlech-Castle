#  Terrain Tronics Harlech Exxample Code
## Wifi Setup with LED's and General Purpose triggers.

**7/1/2022**
A very early version of this document. Let's call it version 0.1

The code in this example is designed to run on a Wemos D1 Processor, developed in the arduino environment, with an assembled Terrain Tronics Harlech Castle board on it.

The Harlech board is designed to run with 8 different LED output that can be controlled from the Wemos D1 Mini. In this case, a few of the channels near the top (nearest the antenna) have been replaced with a pull up resistor of 10KOhm. 

Harlech behaves like a serial to parallel connection, but rather than sending each pin to a high voltage and having the lower pin (the cathode of the LED) have a shared GND, This method is called "high side swithing". Harlech castle works by having a shared high voltage, then using "lower side switches" to switch on and controle the LED's. (known as low side swithing). Thought of in another way, Harlech switches on ground when you send a 1, pulling the voltage at it's pin to ground.

this is commonly done with mechanical switches as it means that one side of the switch is connected to the microcontroller, and the other side can pick up a ground signal from anywhere on the circuit board. easier than trying to find a high voltage!

The pins in the center strip of the Harlech board are your output pins. The strip of pins closest to the IC are either virtually disconnected, or connected to ground (through a fixed current)
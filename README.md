# TerrainTronics-Harlech Castle
8ch LED Driver for Configurable and Programmable Effects
![Cilgerran Castle Intro Picture](https://github.com/user-attachments/assets/df3579ee-54e9-4fa5-a58d-67ecf89b8970)

## Why it exists and the problems it fixes
Harlech is an easy to configure (or code and program) driver for 8 regular LEDs. It talks to the Arduino compatible Wemos D1 Mini below it, takes it's control message and switches the output on and off.
It doesn't need resistors to protect the LED's from blowing, as each output limits it's current, using the lower control knob. If you wire the LED's the wrong way around, nothing will break, they just won't light up.

It also has a keepalive circuit that pulls enough power, every 20 seconds (that's programmable) or so, to keep your USB power bank switched on.

Lots of code examples are here, but you can also download to the Wemos D1, code directly from your browser. It's still in prototype, but you can find it at: https://audio-rochey.github.io/ESP-Web-Tools/

## What does a solution look like?

### Boards

![VerticalStack](https://github.com/user-attachments/assets/601e2180-1c62-4092-af63-614ac53a7921)

A wemos D1 mini acts as the brain. It treats the Harlech Castle board like a shift register - a 74HC595. Any Arduino examples you have with those boards should work.
You'll need a Wemos D1 mini and solder pins of sockets pointing up, then the opposite gender connectors from the Harlech Castle down. You may have to solder the 16 pin header on the Harlech. The Left side of that header is for the Cathode (negative) side of the LED, the right side is for the Anode (the long leg of the LED!)

### Wiring

![Cable Connector Output](https://github.com/user-attachments/assets/994369ca-0bff-40fa-ae99-cdc0f2c93ae0)

I used a 2x8 IDC connector ( [Amazon](https://a.co/d/hbylqzd) ) and ([Amazon](https://a.co/d/8xHjDtx) ) 16pin 1.27mm ribbon cable to handle the outputs from Harlech. 
Doing so allows you build your terrain to a single connector then connect them in one go to the Harlech board. 
Alternatively, if you're mainly familiar with a wire wrapping tool - that works too! - just a little less flexibility later on! :)


## Cool use cases
With it's ability drive 6 high power led's along with 2 more LEDs/ DC Motor from a smart source, existing code examples are available for:

### DMX Controller

[Code Link to come]

I'm addicted to the epic work done by Seb at Atmoseeker ([LINK](https://www.youtube.com/@Atmoseeker)). He controls all of his environmental lights using a stage lighting protocol called DMX. Many ceiling stage lights use DMX as their control method, along with a Ethernet/Wifi-variation called E.131. 
DMX is designed for stage production where you have scenes, so you can do things like change from night to day. Imagine being able to change your ceiling lights from daylight to purples and blues and then having your tabletop buildings switch on their inhouse lights etc!
Example Code in this github, and pre-programmed version available from https://audio-rochey.github.io/ESP-Web-Tools/


### Web Controller

Similar to the DMX controller. This one makes a website on your wifi you can visit.

## What if you Need inputs?
The board is designed to use as many of it's pins as outputs. D1 and D2 are available to use as INPUT, without any modification. Just make sure that you're signals are 3.3V. In addition D3 can be used if you solder the solder jumper to D7. A0 (Analog Input) is connected to the upper control knob.

> Note: If D1 and D2 are both held HIGH (1) or both held LOW (0), the outputs from the MotorA and MotorB pins will be LOW.
Crossed out pins cannot be used as an input, as they have various functions at power on.

## Hacks

### Adding a remote control
D1 or D2 can be used with a 1383 IR Reciever. There's code examples, make sure you tell it which pin you're using! :)

### Power Efficiency: Switch off Wifi if you aren't going to use it!

In Arduino, you can drop the power consumption significantly by adding the following lines in your SETUP function.

```
// Disable the Modem on the ESP8266/Wemos D1 Mini
WiFiMode(WIFI_STA);
WiFi.disconnect(); 
WiFi.mode(WIFI_OFF);
delay(100);
```
These lines drop the current consumption from about 80mA to 14mA.

## How do I buy it?

Cilgerran Castle board comes as both a single board (with connectors) and as a kit, with additional components to make a system.

### What's in the standalone board?
- 1x Harlech Castle Board
- 1x 2x8 Right Angle Connector
  
Your Wemos D1 typically comes with all the connects you need to make a daughter card connect to it!

## Schematics

Version 1P1 schematics are in this folder. Look for the PDF.

# TerrainTronics-Harlech-Castle
Firmware and Schematics for the Harlech Castle 8ch LED controller

https://youtu.be/fndEPuhKIHA

[![Intro Video to Harlech Castle Boards](https://img.youtube.com/vi/fndEPuhKIHA/0.jpg)](https://www.youtube.com/watch?v=fndEPuhKIHA)

A short video of the current PG1.0 schematic is below.

[![Intro Video to Harlech Castle Boards](https://img.youtube.com/vi/SuaxnXeibzg/0.jpg)](https://www.youtube.com/watch?v=SuaxnXeibzg)

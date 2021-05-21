# TerrainTronics-Harlech-Castle-Demo with TableTop Witchcraft

[![Tutorial Video](https://img.youtube.com/vi/Vtr_8sZsUh0/0.jpg)](https://www.youtube.com/watch?v=Vtr_8sZsUh0)

## Introduction
This folder was put together to provide the details and materials for you to clone the terrain that John over at tabletop witchcraft put together in his Awesome video linked above.
The terrain has 8 LED's scattered across it, along with an SSD1306 OLED display that is used to flash a warning sign for the terrain. The 
The terrain uses a common programmable microcontroller board, along with some accessory boards that plug into it to stack. Stacking in this method allows communication and transmission of power between the different boards on a common pinout.

### Ingredients
| Ingredient | Where to Buy | Where to place on terrain |
|-|-|-|
| Wemos D1 Mini Boards | https://amzn.to/3bHn7uZ | In the main housing. |
| Wemos D1 Battery Shield | https://amzn.to/2RrH4z9 | Stacked on Wemos d1. |
| Terrain Tronics Harlech Castle Board | https://www.tindie.com/products/terraintronics/harlech-castle-smart-8xled-controller-pair/ | Top of the stack, connect the LED's to this |
| SSD1306 OLED Display | https://amzn.to/3wlzbtz | Wirewrap to the stack. (can use the long pins on the battery shield) |
| Various 5mm LED's | https://amzn.to/3hIo6z0 | around the terrain! |
| 2x 130mm FlexiLED array. | https://www.tindie.com/products/terraintronics/130mm-flexiled-string/ | Down the pillar sides |
| Blue Flashing LED | https://amzn.to/3bIhblc | behind the fan |
| Lipoly Battery | https://amzn.to/2TbjxmB | Connects to the wemos board. May have a different connector. |

### Tools Needed
| Tools Needed | Where to Buy | Note |
|-|-|-|
| Soldering Tools | https://amzn.to/3hIoH3I | If you don't have this stuff already. |
| Basic Electornics Multimeter | https://amzn.to/2TbiqmV | Stacked on Wemos d1. |
| Wire Wrap Tool | https://amzn.to/3wgNXSq | Expensive, but awesome for wiring stuff together! |
| Wire Wrap Wire | https://amzn.to/3hHcdZX | Lots of wire, lots of colors, makes it easy to wire everything together |
| JST XH Connectors | https://amzn.to/3fDVEM1 | Use these to connect the battery to your wemos battery shield if the connectors don't match. |

I covered a few of these tools in this youtube video: https://www.youtube.com/watch?v=xsE8RqOkm7I

## Harlech board versions and pinouts

There are two versions of the Harlech board. As of 5/21/2021, we have inventory of both. Main difference was a small change in pinout (a software change) and movement of all componenets to the top side of the circuit board to ease manufacturing.
Revisions are based on a X.Y naming structure. X being a major design change (different chips etc) and Y being a small layout change. 
To date, There is a V1.0 and V1.1 - the differences are in the table below.

![Version 1.0 of the Harlech Board](https://i.imgur.com/nufWZZa.png)![Basic Pinout of the Wemos D1 - with thanks to randomnerdtutorials.com](https://i2.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266-WeMos-D1-Mini-pinout-gpio-pin.png?w=715&quality=100&strip=all&ssl=1)

| Pin | Notes about pin | Harlech PG1p0 | Harlech PG1p1 |
|-|-|-|-|
| Features |  | 8out LED | 8out LED |
| D0 | No PWM possible | SPARE | KEEPALIVE |
| D1 | Often used as SCL | I2C-SCL | I2C-SCL |
| D2 | Often used as SDA | I2C-SDA | I2C-SDA |
| D3 | No Pulldown allowed | KEEPALIVE | SPARE (optional OE) |
| D4 | LED. No Pulldown Allowed | !OE | SPARE |
| D5 | Often used for SPI-CLK | SPICLK | SPICLK |
| D6 | Often used for SPI-COPI | SPI-COPI | SPI-COPI |
| D7 | Often used for SPI-CIPO | SPARE | !OE |
| D8 | No Pullup Allowed (Fail Boot). | SPI-CS | SPI-CS |

## Recipe

- Solder short female headers on to your Wemos D1 Mini
- Solder Long-Legged Female headers on to the battery shield.
- Stack (in the same way as the video)
- Wire up. (details below)
- Program (details below, and in the video)



## Technique:

### LED hookups.
Watch https://www.youtube.com/watch?v=SuaxnXeibzg to see how to hook up LED's to the Harlech board. the long leg of your LED's go to the pins on the right hand side (see the video, you'll get it.)
They go in the following order. Channel one is the pair nearest to the Terrain Tronics text at the top of the board.
1. on/off 1 second each state.
2. on constant.
3. on constant
4. on constant
5. random flicker
6. random flicker
7. random flicker
8. random flicker

## SSD1306 OLED Hookup
This is a bit of a hack. Yes. I admit it.
You'll need to wire wrap the following pins from the OLED to the specic pins on the Wemos D1's pins that go up and down through the stack.

![Wiring 3V3, GND and D1/D2 to the BAttery Shield](https://i.imgur.com/tARDRie.png)![Wire Wrap tool to connect from the display](https://i.imgur.com/RURUpVC.png)

| Pin on Display| Pin on the Stack|
|-|-|
| 3v3| 3V3|
| GND| GND|
| SDA| D2|
| SCL| D1|

### 3D Printed Items

![3d printed front panel for the OLED](https://i.imgur.com/GEACved.png)
	
Thingiverse has all my creations on it. You can find the one for the display at: [https://www.thingiverse.com/thing:4861981](https://www.thingiverse.com/thing:4861981 "Grimdark Terrain Panel for SSD1306")


## Downloading the software to the wemos.

Now that you've verified the hardware connectors are all working, it's time to download the software to the WEMOS D1. The Wemos board has non-volative memory onboard, so it remembers the code even when you unplug power.

Downloading the code is covered in the video. 

**REMEMBER** you need to download the image that is connected with your version of Harlech. In this folder there are two BIN files one for version 1p0 of Harlech, the other for version 1p1 of Harlech. Be sure to download the right one for the hardare you have.

[![Tutorial Video](https://img.youtube.com/vi/Vtr_8sZsUh0/0.jpg)](https://youtu.be/Vtr_8sZsUh0?t=221)


## Power

Johns implimentation used a Wemos Battery shield and a small lithium ion battery pack. This works well, but makes your terrain a giant USB battery pack! John had to change the connector on the battery to the XH connector on his battery shield. An inconvenience, but not a show stopper.
It may be easier for many beginners to simply use a USB Battery, the kind used by consumers to charge their smartphones.

## Questions?

Please post on the youtube video, or publish an issue in this github page and I'll do what I can to help! :)


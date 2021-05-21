# TerrainTronics-Harlech-Castle-Demo with TableTop Witchcraft

**https://youtu.be/fndEPuhKIHA**


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



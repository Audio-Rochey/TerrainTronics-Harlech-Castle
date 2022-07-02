#  Terrain Tronics Harlech Exxample Code
## Wifi Setup with LED's and General Purpose triggers.

**7/1/2022**

A very early version of this document. Let's call it version 0.1

The code in this example is designed to run on a Wemos D1 Processor, developed in the arduino environment, with an assembled Terrain Tronics Harlech Castle board on it.

The Harlech board is designed to run with 8 different LED output that can be controlled from the Wemos D1 Mini. In this case, a few of the channels near the top (nearest the antenna) have been replaced with a pull up resistor of 10KOhm. 

Harlech behaves like a serial to parallel connection, but rather than sending each pin to a high voltage and having the lower pin (the cathode of the LED) have a shared GND, This method is called "high side swithing". Harlech castle works by having a shared high voltage, then using "lower side switches" to switch on and controle the LED's. (known as low side swithing). Thought of in another way, Harlech switches on ground when you send a 1, pulling the voltage at it's pin to ground.

this is commonly done with mechanical switches as it means that one side of the switch is connected to the microcontroller, and the other side can pick up a ground signal from anywhere on the circuit board. easier than trying to find a high voltage!

The pins in the center strip of the Harlech board are your output pins. The strip of pins closest to the IC are either virtually disconnected, or connected to ground (through a fixed current).


# How the code works.

To learn how to download/compile code to your Harlech board, watch this video: https://www.youtube.com/watch?v=TgE-Op0UvxE

You'll also need a few additional libraries. WifiManager, WebSocketServer and Double Reset Detector are the ones that come to mind.

The first time you download the code, the board will reset and then setup it's own WiFi network for you to connect from your phone/computer, to teach it about your home network. **If you use the serial monitor that is built into the Arduino Development tools, you can watch information come back from the board. Useful to see what's being sent to the board and what's going on with the server!**

Once you've configured the board for your wifi network, you'll need to find out which IP address it was given. this can be done using your wifi routers internal webpage, or looking the data on the arduino monitor. you should be able to find the server on your network at http://TerrainTronics01/

From there, a html file is brought up on your mobile or your pc with a few buttons, similar to the following:


# How to modify the code - the basics

The code uses *websockets* to communicate from your browser (whether it's a phone or a pc, or anything else). Websockets are a streamlined way of communicating with servers. The htmlfile.h describes the file that is sent to your browser when you connect. 
To make a new button pop up on your GUI, then you need to add an additional line to this text.
'<button id='BTN_LEDA'class="button">Trigger Sound Effect board</button>
      <button id='BTN_LEDB'class="button">Lights On</button>
      <button id='BTN_LEDC'class="button">Lights Off</button>
      <button id='BTN_LEDD'class="button">Run LED Pattern</button>'


That handles the bit that's shown, then we need to teach it what do when one of those buttons is pressed... so a little further down at: 

'document.getElementById('BTN_LEDB').addEventListener('click', buttonBClicked);
  function buttonBClicked()
  {   
    sendText('1');
  }'






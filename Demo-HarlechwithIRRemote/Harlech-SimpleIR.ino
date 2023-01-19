//**************************************************************//
//  Name    : Harlech Castle PG1.0 Simple IR                    //
//  Author  : Dafydd Roche                                      //
//  Date    : 1/18/2023                                         //
//  Version : 1.0                                               //
//  Notes   : This code runs on the Wemos D1 Mini.              //
//            This code does not use any of the Wifi            //
//              Capabilities                                    //
//****************************************************************

/* Simple Mods possible on Harlech Castle board.
 * 
 * Use WireWrap to connect the IR Reciever to 3.3V, GND and D3.
 * Then can we WW'd to the pin headers on Wemos Board, then stack the Harlech board on top.
 * 
 * 
 * 
 * Cheers
 * 
 * TerrainTronics
 * 
 * 
 */





#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
extern "C" {
#include "user_interface.h"
}
#include<math.h>

const String softwareVersion = "V1.0";
const String softwareDate = "1/17/2023";     
const String HarlechVer = "1p1";

// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;// The Keepalive pin for USB Power Banks is D0. UPdates in Harlech 1.1
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.

// #define DEBUG 0 // this is used to switch debug comments on and off. Comment it out with // to optimize the code.

 
// These constants won't change. They're used to give names to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to.

int patternData = 0;
int dutyCycle = 1023;  

//These definitions configure the pins for the SPI interface to talk to the LED driver
int latchPin = D8; // Checked with PG1.1 of Harlech
int clockPin = D5; // Checked with PG1.1 of Harlech
int dataPin = D6; // Checked with PG1.1 of Harlech
int OE = D7; // Set the output enable pin. This can be rapidly PWM'd. Checked with PG1.1 of Harlech. (Soldered to lower 2)
int OEBrightness = 0;
int OEBrightnessIndex = 1;

const uint16_t kRecvPin = D3;
IRrecv irrecv(kRecvPin);
decode_results results;



//=======================================================================
// This is a subroutine that switches the keepAlive pin on/off to switch on a transistor that pulls current to ground, through a 47Ohm resistor
// PLEASE NOTE: You cannot leave this on for a long long time, as the resistor will begin to heat up beyond it's rating!

void keepAliveChangeState()
{
  //Serial.println("Keepalive Change");
  if (USBBankState == 0 ){
    //Serial.println("Switching off");
    USBBankState = 1;
    digitalWrite(keepAlive, 0); // Switch Transistor Off
    USBBank.attach(kaTimeOff, keepAliveChangeState); //Use attach_ms   
  }
  else if (USBBankState == 1 ){
    //Serial.println("Switching on");
    USBBankState = 0;
    digitalWrite(keepAlive, 1); // Switch Transistor On
    USBBank.attach(kaTimeOn, keepAliveChangeState); //Use attach_ms   
  }
}




void setup() {
  
  WiFi.disconnect();     // We're not using Wifi, so lets disable it to save power.
  WiFi.forceSleepBegin();
  delay(1); //For some reason the modem won't go to sleep unless you do a delay
  
  
  

  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  delay(1000);
  Serial.println("*****");
  delay(100);
  Serial.println("TerrainTronics Demo - Generic Remote Control Standard");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date of Compile ");
  Serial.print(softwareDate);
  Serial.print(" Harlech Hack HW Version ");
  Serial.println(HarlechVer);
  Serial.print("IRrecv: ");
  Serial.println(kRecvPin);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!

  USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.

  patternData = 0xFF;
  OEBrightness = 1;
}

void callback() {
  Serial.println("WAKE FROM IR!");
  irrecv.enableIRIn(); // Reconfigures timers etc on the device to accept IR (e.g. timer to 50uS to sample the input etc)

}

void loop() {
      //Serial.println("Loop");
    int allOFF = 0;
    int bitMask = 0x00;
    
    if (irrecv.decode(&results)) {
      Serial.print("IR Recieve: ");
      serialPrintUint64(results.value, HEX);
      Serial.println("");

      switch (results.value) { 
        case 0xFFE01F: // Button - (make dimmer)
        // Make number higher, as level is inverted. (0=bax brightness, 255 = lowest)
        // starts at 1
        //Serial.print("Old Brightness Index = ");
        //Serial.println(OEBrightnessIndex);
        OEBrightnessIndex = OEBrightnessIndex + 2;
        if (OEBrightnessIndex > 10){
          OEBrightnessIndex = 10;
          //Serial.print("Old Brightness Index Limited to 10 ");          
        }
        OEBrightness = (log10(OEBrightnessIndex) * 255);
        Serial.println(OEBrightness);
        break;

        case 0xFFA857: // Button + (Make brighter)
        //Serial.print("Old Brightness Index = ");
        //Serial.println(OEBrightnessIndex);
        OEBrightnessIndex = OEBrightnessIndex - 2;
        if (OEBrightnessIndex < 1){
          OEBrightnessIndex = 1;
          Serial.print("Old Brightness Index Limited to 1 ");          
        }
        OEBrightness = (log10(OEBrightnessIndex) * 255);
        Serial.println(OEBrightness);
        break;

        case 0xFF6897: // 0 button - all off.
        allOFF = 1;
        break;

        case 0xFF30CF: // 1 button - LED1 Toggle.
        bitMask = 0b00000001;
        break;

        case 0xFF18E7: // 2 button - LED2 Toggle.
        bitMask = 0b00000010;
        break;

        case 0xFF7A85: // 3 button - LED3 Toggle.
        bitMask = 0b00000100;
        break;

        case 0xFF10EF: // 4 button - LED4 Toggle.
        bitMask = 0b00001000;
        break;

        case 0xFF38C7: // 5 button - LED5 Toggle.
        bitMask = 0b00010000;
        break;

        case 0xFF5AA5: // 6 button - LED6 Toggle.
        bitMask = 0b00100000;
        break;

        case 0xFF42BD: // 7 button - LED7 Toggle.
        bitMask = 0b01000000;
        break;

        case 0xFF4AB5: // 8 button - LED8 Toggle.
        bitMask = 0b10000000;
        break;

        case 0xFF52AD: // 8 button - LED8 Toggle.
        allOFF = 2;
        break;

        
      }

      
   
   irrecv.resume();  // Receive the next value
    }

    if (allOFF == 1){
      patternData = 0x00;
      allOFF = 0;
    }
    if (allOFF == 2){
      patternData = 0xFF;
      allOFF = 0;
    }
    else {
      patternData = patternData ^ bitMask; // this takes any bitmasks set by the code above and toggles the existing output.
    }
    // then this clocks the data out to LED driver chip.
    digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
    shiftOut(dataPin, clockPin, LSBFIRST, patternData);
    digitalWrite(latchPin, HIGH); // Tells the LED driver 
   // Serial.println("DONE WRITING");
   analogWrite(OE, OEBrightness);

}

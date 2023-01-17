//**************************************************************//
//  Name    : Harlech Castle PG1.1 PWM Patterns                 //
//  Author  : Dafydd Roche                                      //
//  Date    : 11/15/22                                          //
//  Version : 1.0                                               //
//  Notes   : This code runs on the Wemos D1 Mini.              //
//            This code does not use any of the Wifi            //
//              Capabilities                                    //
//****************************************************************

/* Simple Mods possible on Harlech Castle board.
 * Things you need to do when you get your hands on this code.
 * - Configure the Keepalive settings. See the lines "const int kaTimeOn" and "kaTimeOff".
 * This circuits pulls a burst of power for 1 second every 8 seconds by default. This needs to be tuned 
 * for the USB battery pack you are using. Some might only need it for 0.5 seconds every 30 seconds 
 * to make sure it stays on. Others, need it WAY more often. 1 second on every 8 seconds was selected with
 * the crappy battery pack I was using
 * 
 * Variable brightness using harlech is done in a brute force method.
 * the shift register is updated thousands of times a second, and the ratio time an 
 * LED is ON vs Off is used. This allows the update rate of the shift register to vary with code used, and providing
 * it's updated fast enoough for the eye to be fooled, it should look the same.
 * For example, L1ResetCount is used to say "switch on 1 in 50". The counter will start at 50, update the shift register with
 * a zero in that bit, then subtract one and loop, until it gets to zero. As soon as zero is hit, it'll change that bit to a 1, 
 * just for that update, and reset the counter.
 * 
 * alwaysOnLEDS = 0b11110000; is used to switch on a bunch of LED's permanently. (the 4 bottom ones near the USB connector)
 * 
 * Any questions, come on over to github or post on the facebook group!
 * 
 * Cheers
 * 
 * TerrainTronics
 * 
 * PG1.1 updates:
 * Keepalive is now on pin D0.
 * 
 */
#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!

// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;// The Keepalive pin for USB Power Banks is D0.
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.

// #define DEBUG 0 // this is used to switch debug comments on and off. Comment it out with // to optimize the code.

 
// These constants won't change. They're used to give names to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to.



//These definitions configure the pins for the SPI interface to talk to the LED driver
int latchPin = D8;
int clockPin = D5;
int dataPin = D6;
// Set the output enable pin. This can be rapidly PWM'd.
int OE = D7;




int L1ResetCount; // used for internal counter PWM of L1
int L1Count; // used for internal counter PWM of L1
bool L1DIR; // Are we breathing UP or DOWN? Used to track previous status of counter or LED.
int L1Min = 400 ; // Lowest Brightness. The means the LED would be ON for 1 in X updates. (e.g. 100 would be 1% Duty cycles)
unsigned long L1previousMillis = 0;  // will store last time LED brightness was updated
long L1interval = 100; // Update delay between changes in brightness


int L2ResetCount; // used for PWM of L2
int L2Count; // used for PWM of L2 (counter, how many to go until we get to 0, when the LED switches on?)
bool L2DIR; // Are we breathing UP or DOWN?
int L2Min = 50 ; // Lowest Brightness. The means the LED would be ON for 1 in X updates. (e.g. 100 would be 1% Duty cycles)
unsigned long L2previousMillis = 0;  // will store last time LED was updated
long L2interval = 15;  //Update delay

int L3ResetCount; // used for PWM of L3
int L3Count; // used for PWM of L3
bool L3DIR = LOW; // Are we breathing UP or DOWN? Used to track previous status of counter or LED.
int L3Min = 200 ; // Lowest Brightness. The means the LED would be ON for 1 in X updates. (e.g. 100 would be 1% Duty cycles)
unsigned long L3previousMillis = 0;  // will store last time LED was updated
long L3interval = 15;  //Update delay

int L4ResetCount; // used for PWM of L4
int L4Count; // used for PWM of L4
bool L4DIR = LOW; // Are we breathing UP or DOWN? Used to track previous status of counter or LED.
int L4Min = 200 ; // Lowest Brightness. The means the LED would be ON for 1 in X updates. (e.g. 100 would be 1% Duty cycles)
unsigned long L4previousMillis = 0;  // will store last time LED was updated
long L4interval = 15;  //Update delay


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
  
  
  // initialize serial communications at 9600 bps: this is only used for communication with a PC.
  Serial.begin(115200);
    //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!

  USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.

  analogWrite(OE, 0);


  // Just a few initial values.
  L1ResetCount = L1Min;
  L1Count = L1ResetCount;
  L2ResetCount = L2Min;
  L2Count = L2ResetCount;
  L3ResetCount = L3Min;
  L3Count = L3ResetCount;
  L4ResetCount = L4Min;
  L4Count = L4ResetCount;

}

void loop() {


    // These handle the Brightness updates.
    // The values sent updated are L*ResetCount.
    // A pulse happens when resetcount=0. 
    // So a resetcount of 10 means that the LED is on for 1 pulse every 10 etc.
   unsigned long currentMillis = millis();
   if (currentMillis - L1previousMillis >= L1interval) {
      L1ResetUpdateCall(currentMillis);
   }
      if (currentMillis - L2previousMillis >= L2interval) {
      L2ResetUpdateCall(currentMillis);
   }
      if (currentMillis - L3previousMillis >= L3interval) {
      L3ResetUpdateCall(currentMillis);
   }
      if (currentMillis - L4previousMillis >= L4interval) {
      L4ResetUpdateCall(currentMillis);
   }



    // When each count is zero (usually with flickering or breathing), then the LED is ON.
    // sooon as it's raised higher (by a LxResetUpdateCall) then it switches off.
    // Management of these changing the Count values should only be done in the appropriate UpdateCall.
   byte L1Mask;
   if (L1Count == 0){
    L1Mask = 0b00000001;
    L1Count = L1ResetCount;
   }
   else {
    L1Mask = 0b00000000;
    L1Count--;
   }
   byte L2Mask;
   if (L2Count == 0){
    L2Mask = 0b00000010;
    L2Count = L2ResetCount;
   }
   else {
    L2Mask = 0b00000000;
    L2Count--;
   }
   byte L3Mask;
   if (L3Count == 0){
    L3Mask = 0b00000100;
    L3Count = L3ResetCount;
   }
   else {
    L3Mask = 0b00000000;
    L3Count--;
   }
   byte L4Mask;
   if (L4Count == 0){
    L4Mask = 0b00001000;
    L4Count = L4ResetCount;
   }
   else {
    L4Mask = 0b00000000;
    L4Count--;
   }

   byte totalLedMask;
   byte alwaysOnLEDS = 0b11110000;
   totalLedMask = (0b00000000 | alwaysOnLEDS | L1Mask | L2Mask | L3Mask | L4Mask); // Bitwise OR to set those lights high.
   shiftToLed(totalLedMask);



}
// This shiftToLed function simply pushes the data out the serial port.
void shiftToLed (byte patternData){
   digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
   shiftOut(dataPin, clockPin, LSBFIRST, patternData);
   digitalWrite(latchPin, HIGH); // Tells the LED driver 
}

void L1ResetUpdateCall(unsigned long currentMillis){
    //This example shows breathing. The "DIR" makes L1ResetCount move up every time it's called.
    // How quickly each step takes is in one of the constants at the top.
    L1previousMillis = currentMillis;
    L1interval = 15;
    if (L1DIR == LOW) {
          L1ResetCount --;
          if (L1ResetCount == 0){
            L1DIR = HIGH;
          }
        } else {
          L1ResetCount++;
          if (L1ResetCount == L1Min){
            L1DIR = LOW;
          }
        }
}

void L2ResetUpdateCall(unsigned long currentMillis){
      // Example of random candle style flicker.
    L2previousMillis = currentMillis;
    L2interval = random(20,200); // delay between changes in brightness
    L2ResetCount = random(1,L2Min); // the actual brightness.
}

void L3ResetUpdateCall(unsigned long currentMillis){
    // ON and OFF Switching.
    // L3DIR in this mode simply tracks the previous state. Low means it WAS off (and when it runs, it'll go "HIGH") 
    //Serial.println("L3ResetUpdateCall");
    L3previousMillis = currentMillis;
    L3interval = 100; // changes the flickering rate. every 3 seconds current.

    if (L3DIR == LOW) {
        //Serial.println("ON");
        L3ResetCount = 0; // switch it to full on brighness and wait 100mS.
        L3DIR = HIGH;     // Then next time it rolls around it'll jump to the next else statement.
     } else {   // so this is if L3DIR == High.
       //Serial.println("OFF");
       L3ResetCount = L3Min; // There's no such thing as OFF, just a duty cycle so low the eye can't see it.
       L3DIR = LOW;
     }
     //Serial.println(L3ResetCount);
}

void L4ResetUpdateCall(unsigned long currentMillis){
      // ON and OFF Switching. 
    //Serial.println("L3ResetUpdateCall");
    L4previousMillis = currentMillis;
    

    if (L4DIR == LOW) { // If the LED is OFF.
        //Serial.println("ON");
        L4interval = random(3000,5000); // delay between flickers (some time between 3 and 5 seconds)
        if (random(0,3) == 0){
          L4interval = 25;              // one in 4 chance that we could get a double re-eval of the flicker.
        }
        L4ResetCount = 0; // full on brighness
        L4DIR = HIGH; // LED ON
     } else {           // If the LED in ON. 
       //Serial.println("OFF");
       L4interval = random(20,50); // delay between changes in brightness
       L4ResetCount = random(1,L4Min); // the actual brightness.
       L4DIR = LOW;
     }
}

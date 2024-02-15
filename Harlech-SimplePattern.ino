//**************************************************************//
//  Name    : Harlech Castle PG1.0 Simple Patterns              //
//  Author  : Dafydd Roche                                      //
//  Date    : 2/8/2021                                         //
//  Version : 1.2                                               //
//  Notes   : This code runs on the Wemos D1 Mini.              //
//            This code does not use any of the Wifi            //
//              Capabilities                                    //
//****************************************************************

/* Simple Mods possible on Harlech Castle board.
 *  Connect Pin2 to pin3 of Pot R2 to make sure the potentiometer changes the constant current.
 *  PWM'ing the OE pin can do amplitude control
 *  That circuit is normally used to keep a USB Powerbank alive by pulling a load once in a while.
 *
 * 
 * Things you need to do when you get your hands on this code.
 * - Configure the Keepalive settings. See the lines "const int kaTimeOn" and "kaTimeOff".
 * This circuits pulls a burst of power for 1 second every 8 seconds by default. This needs to be tuned 
 * for the USB battery pack you are using. Some might only need it for 0.5 seconds every 30 seconds 
 * to make sure it stays on. Others, need it WAY more often. 1 second on every 8 seconds was selected with
 * the crappy battery pack I was using
 * 
 * Select which pattern you want. There are three patterns in this code, the line of code starts with 
 * int ledPattern[][3]. The default one is all on, all off for 3 seconds each. 2 states and it just goes 'round and round.
 * To select the one you want, comment out the test one (by using // in front of the lines) and uncomment the pattern you want.
 * (Simply delete the // in front of the lines you want)
 * 
 * Any questions, come on over to github or post on the facebook group!
 * 
 * Cheers
 * 
 * TerrainTronics
 * 
 * UPDATE LOG
 * Feb 15th 2024 -- Updated KeepAlive Pin and OE pin to use the right pins on Harlech 1.1
 */
#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!

// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;// HARLECH 1.1 The Keepalive pin for USB Power Banks is D0.
//const int keepAlive = D3;// HARLECH 1.0 The Keepalive pin for USB Power Banks is D3.
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
// int OE = D4; // D4 is only for Harlech 1.0
int OE = D7; // D7 is the default on Harlech 1.1, or it can be changed to D3.




int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output to the PWM (analog out)
int dutyCycle = 1023;          // set starting - PWM Duty Cycle. Data is inverted so 1023 is off.
int flashdelay = 500;
int breathDir = 0;  //Breathing Direction - 0 is counting up, 1 is counting down

/*
 * The following array defines patterns. 
 * The first column sets which LED's are on or off. 
 * The second column defines the brightness. 0 is full on, 1023 is full off.
 * The third column defines how many miliseconds they should be on for.
 * The array can have many many steps, with short timing between them. Worst case, you'll run out of memory on the chip (160KBytes worth!).
 */




//LED Test Pattern. 
//Standard LED Test Pattern
//This the pattern (on/off flashing) thats used in production to test the pins are soldered correctly etc.
int ledPattern[][3] = {   {0b00000011,  0,  3000 } ,
                          {0b00001100,  0,  3000 } };

//Candle Flicker (on one of the LED's)
//The brightness (second column) is changed in each step as well as the time between steps (3rd column)
//Technically, this could be done with a clever randomizer algorithm. Left as an excercise to the reader! ;)

//int ledPattern[][3] = {   {0b00000111,  600,  100 } ,
//                          {0b00000111,  300,  150 } ,
//                          {0b00000111,  100,  200 } ,
//                          {0b00000111,  0,  100 } ,
//                          {0b00000111,  400,  100 } ,
//                          {0b00000111,  800,  50 } ,
//                          {0b00000111,  1000,  200 } ,
//                          {0b00000111,  1020,  50 } ,
//                          {0b00000111,  850,  150 } ,
//                          {0b00000111,  900,  100 } ,
//                          {0b00000111,  500,  150 } ,
//                          {0b00000111,  100,  200 } ,
//                          {0b00000111,  0,  100 } ,
//                          {0b00000111,  600,  100 } ,
//                          {0b00000111,  800,  50 } ,
//                          {0b00000111,  100,  200 } ,
//                          {0b00000111,  1020,  50 } ,
//                          {0b00000111,  850,  150 } ,
//                          {0b00000111,  900,  100 } ,
//                          {0b0000011 1,  400,  200 } };


//int ledPattern[][3] = {   {0b11111111,  0,  500 } ,
//                          {0b00000000,  0,  500 } ,
//                          {0b11111111,  0,  500} ,
//                          {0b00000000,  0,  500 } ,
//                          {0b10001000,  0,  250 } ,
//                          {0b01000100,  0,  250 } ,
//                          {0b00100010,  0,  250 } ,
//                          {0b00010001,  0,  250 } ,
//                          {0b00000000,  0,  1000 } };



// Set Up and initialize the pointers used for patterns                          
int patternIndex = 0;
int patternData = 0;
int patternDelay = 0;
int numrows = 0;

int numRows = sizeof(ledPattern)/sizeof(ledPattern[0]);

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
  Serial.begin(9600);
    //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!

  USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.

}

void loop() {


// Defines the three variables
  patternData = ledPattern[patternIndex][0];
  dutyCycle = ledPattern[patternIndex][1];
  patternDelay = ledPattern[patternIndex][2];


/*
 * This little bit allows you to print debuggin data back up the serial port to the PC. If DEBUG is not defined above, or not commented out, then it'll print.
 */
#ifdef DEBUG  
  Serial.println("Pattern");
  Serial.println(patternIndex);
  Serial.println(patternData);
  Serial.println(dutyCycle);
  Serial.println(patternDelay);
#endif
  
   // Serial.println("WRITE TO SHIFTER");
    digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
    shiftOut(dataPin, clockPin, LSBFIRST, patternData);
    digitalWrite(latchPin, HIGH); // Tells the LED driver 
   // Serial.println("DONE WRITING");
   analogWrite(OE, dutyCycle);
   delay(patternDelay);                        // HERE IS THE ONE "DELAY" CALL. DELAY is a lazy command. Ideally, you'd set the processor into deep sleep for this time, rahter than have it twiddle its thumbs.

  patternIndex++; // Step to the next item in the index
  if (patternIndex == numRows) {    // if you hit the maximum number of rows in teh pattern, set it back to the 0'th row (the first)
    patternIndex = 0;
  }


}

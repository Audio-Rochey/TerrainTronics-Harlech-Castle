//**************************************************************                //
//  Name    : Harlech and a SSD1306 OLED Display at the same time!              //
//  Author  : Dafydd Roche                                                      //
//  Date    : 5/10/2021                                                         //
//  Version : 1.1                                                               //
//  Notes   : This code runs on the Wemos D1 Mini.                              //
//            This code does not use any of the Wifi                            //
//              Capabilities                                                    //
//            SSD1306 runs from the 3V3 rail and D1 and D2 for I2C              //
//            D5, D6, D8 are used for the SPI chain running into the LED driver.//
//            Output Enable is D4 and used (and PWM'd)                          //
//            This code example isn't the best code in the world. It just       //
//            about works. It uses a LOT of Adafruit code examples!             //
//********************************************************************************




#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #define KeepAliveEnable // uncommenting this line will switch on the keepalive timer etc. 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// #define DEBUG 0 // this is used to switch debug comments on and off. Comment it out with // to optimize the code.



// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library

Ticker displayInvert;
Ticker USBBank;

int USBBankState = 0;
const int keepAlive = D3;// The Keepalive pin for USB Power Banks is D3.
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 9; // Time to switch the keep alive circuit off for.
 
// These constants won't change. They're used to give names to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to.
const int latchPin = D8; //These definitions configure the pins for the SPI interface to talk to the LED driver
const int clockPin = D5;
const int dataPin = D6;
const int OE = D4; // Set the output enable pin. This can be rapidly PWM'd.


int outputValue = 0;        // value output to the PWM (analog out)
int dutyCycle = 1023;          // set starting - PWM Duty Cycle. Data is inverted so 1023 is off.


/*
 * The following array defines patterns. 
 * The first column sets which LED's are on or off. 
 * The second column defines the brightness. 0 is full on, 1023 is full off.
 * The third column defines how many miliseconds they should be on for.
 * The array can have many many steps, with short timing between them. Worst case, you'll run out of memory on the chip (160KBytes worth!).
 */


int ledPattern[][3] = {   {0b11111101,  0,  250 } ,
                          {0b11111101,  0,  250 } ,
                          {0b00111101,  0,  20 } ,
                          {0b11111101,  0,  230 } ,
                          {0b10111101,  0,  250 } ,
                          {0b01111111,  0,  25 } ,
                          {0b11111111,  0,  225 } ,                          
                          {0b01111111,  0,  25 } ,
                          {0b00111111,  0,  225 } ,
                          {0b11011111,  0,  225 } ,
                          {0b01011111,  0,  25 } ,
                          {0b11011101,  0,  250 } ,
                          {0b01011001,  0,  50 } , //Flciker #3 off. (look how the time of this one - 50 and the next (200) add up!
                          {0b01111101,  0,  200 } ,
                          {0b11111101,  0,  250 } ,
                          {0b11111101,  0,  250 } ,
                          {0b01111111,  0,  250 } ,                          
                          {0b11011111,  0,  250 } ,
                          {0b10011011,  0,  250 } , // Longer flicker off.
                          {0b00111111,  0,  250 } };


// Set Up and initialize the pointers used for patterns                          
int patternIndex = 0;
int patternData = 0;
int patternDelay = 0;
int numrows = 0;

int numRows = sizeof(ledPattern)/sizeof(ledPattern[0]);

bool displayinvertstate = false; //global variable for the state of displayinversion


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

  #ifdef KeepAliveEnable
    USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.
  #endif
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...


 

 terrainscreen();      // Draw characters of the default font
 displayInvert.attach(1, invertScreen);

}

void loop() {


// Defines the three variables
  patternData = ledPattern[patternIndex][0];
  dutyCycle = ledPattern[patternIndex][1];
  patternDelay = ledPattern[patternIndex][2];


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

void terrainscreen(void) {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, WHITE);
  
  display.fillRoundRect(3, 5, 123, 20, 4, WHITE);
  
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(6,7);             // Start at top-left corner
  display.println(F("GEN STATUS"));

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(48,30);             // Start at top-left corner
  display.println(F("LOW"));
  display.setCursor(38,48);             // Start at top-left corner
  display.println(F("POWER"));

  display.display();
}

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

void invertScreen(){
  
  display.invertDisplay(displayinvertstate);
  displayInvert.attach(0.5, invertScreen);
  displayinvertstate = !displayinvertstate;
}

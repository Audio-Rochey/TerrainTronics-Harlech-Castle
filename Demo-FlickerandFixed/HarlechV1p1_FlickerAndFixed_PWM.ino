//**************************************************************//
//  Name    : Harlech Castle Solid & Candle Flicker             //
//  Author  : Dafydd Roche                                      //
//  Date    : 2/2/2025                                          //
//  Version : 1.1 (Software SPI Version)                      //
//  Notes   : This code runs on the Wemos D1 Mini using         //
//            optimized software SPI with a fixed hardware config//
//****************************************************************

/* 
Using the A0 potentiometer, select one of 4 patterns:

6 Programs split the outputs into 2 banks.
All OFF
4xON, 4xOFF
4x CANDLE, 4x OFF
8x CANDLE
8x ON
4x CANDLE, 4x ON
8x ON

Control is via the control knob near the top of the board. Total brightness is controlled using the lower knob.
The control knob can be replaced with an external potentiometer (knob) - 10KOhm, and added to your terrain.

This lets you drive either flickering (candle) LEDs or steady ones.

ChatGPT provided an optimized SPI function for sending data to Harlech. It's faster than "Shiftout" as it does less if-this-then-thatting... as it knows the chip it's talking to.
*/

#include <ESP8266WiFi.h>  // Used here only to disable WiFi and save power.
#include <Arduino.h>
extern "C" {
  #include "user_interface.h"
}
#include <math.h>
#include <Ticker.h>      // Ticker Library

// --- Direct GPIO register access macros for the ESP8266 ---
// These macros allow fast pin manipulation by writing directly to the registers.
#ifndef GPIO_REG_WRITE
  #define GPIO_REG_WRITE(addr, val) (*(volatile uint32_t *)(addr) = (val))
#endif

#ifndef GPIO_OUT_W1TS_ADDRESS
  #define GPIO_OUT_W1TS_ADDRESS 0x60000304  // Write 1 to Set bits.
#endif

#ifndef GPIO_OUT_W1TC_ADDRESS
  #define GPIO_OUT_W1TC_ADDRESS 0x60000308  // Write 1 to Clear bits.
#endif

// --- Define pin masks for our software SPI ---
// We want to use D6 (GPIO12) as MOSI and D5 (GPIO14) as Clock.
#define MOSI_MASK (1 << 12)  // D6 is GPIO12.
#define CLK_MASK   (1 << 14)  // D5 is GPIO14.

const String softwareVersion = "V1.0";
const String softwareDate    = "2/2/2025";     
const String HarlechVer     = "1p1";

// --- USB Bank Keep-Alive functionality ---
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;  // Keepalive pin for USB power banks.
const int kaTimeOn  = 1;   // Time (in seconds) to switch the keepalive circuit on.
const int kaTimeOff = 9;   // Time (in seconds) to switch it off.

#define DEBUG 0

// --- Analog input configuration ---
const int analogInPin = A0;  // Potentiometer input pin.
unsigned long previousMillisADC; // For ADC timing.

int dutyCycle = 1023;  

// --- LED Driver Pin Definitions for Software SPI ---
// We will use the following connections:
//   - MOSI (data)  : D6 (GPIO12)
//   - Clock (SCLK) : D5 (GPIO14)
//   - Latch        : D8
//   - OE (Output Enable) remains on D7.
const int latchPin = D8;  // Latch for the LED driver.
const int OE       = D7;  // Output Enable for the LED driver.

// Global variables for LED output and PWM:
int OEBrightness = 0;
int range;                   // ADC-derived range value.
byte LEDOutputBuffer = 0;    // Each bit represents one LED’s desired state.
byte pwmCounter = 0;         // PWM counter (cycles from 0 to 255).

// --- LED Modes ---
enum ledModes {
  LED_OFF,    // LED off.
  SOLID_ON,   // LED steadily on.
  CANDLE_ON   // Flickering (candle) mode.
};

// --- Class to Manage an Individual LED Channel ---
class CandleorFixed {
  byte bufferPosition;       // Bit mask for this LED.
  int newVal;                // Not used further in this example.
  int lowPassTrack;          // Used for smoothing brightness changes.
  unsigned long previousMillis; // For timing updates.
  int pwmValue;              // Calculated PWM brightness (0–255).
  ledModes channelMode;      // Current mode for this LED.
  
public:
  CandleorFixed(byte bufferPositionin, ledModes channelModeIn)
    : bufferPosition(bufferPositionin),
      channelMode(channelModeIn),
      newVal(0),
      lowPassTrack(0),
      previousMillis(0),
      pwmValue(0)
  {}

  // Update the LED mode if necessary.
  void setMode(ledModes channelModeIn) {
    channelMode = channelModeIn;
  }

  // Update the PWM value and adjust the LED output buffer accordingly.
  void Update() {
    unsigned long currentMillis = millis();
    switch (channelMode) {
      case LED_OFF:
        pwmValue = 0;
        break;
      case SOLID_ON:
        pwmValue = 255;
        break;
      case CANDLE_ON:
        if (currentMillis - previousMillis >= 100) {  // Update every 100 ms.
          // Random flicker: a random value between 0 and 127 plus half the previous value.
          pwmValue = random(128) + (lowPassTrack / 2);
          lowPassTrack = pwmValue;
          previousMillis = currentMillis;
        }
        break;
    }
    
    // Use the pwmCounter (0–255) to decide whether to set or clear this LED's bit.
    if (pwmCounter < pwmValue) {
      LEDOutputBuffer |= bufferPosition;   // Turn LED on.
    } else {
      LEDOutputBuffer &= ~bufferPosition;    // Turn LED off.
    }
  }
};

// --- Define 8 LED channels using bit masks ---
CandleorFixed LEDA(0b00000001, LED_OFF);
CandleorFixed LEDB(0b00000010, LED_OFF);
CandleorFixed LEDC(0b00000100, LED_OFF);
CandleorFixed LEDD(0b00001000, LED_OFF);
CandleorFixed LEDE(0b00010000, LED_OFF);
CandleorFixed LEDF(0b00100000, LED_OFF);
CandleorFixed LEDG(0b01000000, LED_OFF);
CandleorFixed LEDH(0b10000000, LED_OFF);

// --- Optimized Software SPI Transfer Function ---
// This function sends one byte (LSBFIRST) by directly manipulating the GPIO registers.
inline void softSPITransfer(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    // Set MOSI (D6) according to the least significant bit of data.
    if (data & 0x01) {
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, MOSI_MASK);  // Set MOSI high.
    } else {
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, MOSI_MASK);  // Set MOSI low.
    }
    // Pulse the clock (D5):
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, CLK_MASK);  // Set Clock high.
    __asm__ __volatile__("nop"); // Optional very-short delay.
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, CLK_MASK);  // Set Clock low.
    
    data >>= 1; // Shift to the next bit.
  }
}

// --- USB Bank Keepalive Toggle Function ---
void keepAliveChangeState() {
  if (USBBankState == 0) {
    USBBankState = 1;
    digitalWrite(keepAlive, 0); // Turn transistor off.
    USBBank.attach(kaTimeOff, keepAliveChangeState);
  } else {
    USBBankState = 0;
    digitalWrite(keepAlive, 1); // Turn transistor on.
    USBBank.attach(kaTimeOn, keepAliveChangeState);
  }
}

void setup() {
  // Disable WiFi to save power.
  WiFi.disconnect();
  WiFi.forceSleepBegin();
  delay(1);
  
  // Set CPU frequency to 160 MHz.
  system_update_cpu_freq(160);
  
  Serial.begin(115200);
  while (!Serial)
    delay(50);
  Serial.println();
  delay(1000);
  Serial.println("*****");
  delay(100);
  Serial.println("TerrainTronics - Test Program for Harlech (Software SPI)");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date ");
  Serial.print(softwareDate);
  Serial.print(" HW Version ");
  Serial.println(HarlechVer);
  Serial.print("IRrecv: NONE ");

  // Configure pin modes.
  pinMode(latchPin, OUTPUT); // Latch for LED driver.
  pinMode(OE, OUTPUT);       // Output Enable for LED driver.
  pinMode(keepAlive, OUTPUT);
  digitalWrite(keepAlive, LOW);
  USBBank.attach(1, keepAliveChangeState);
  
  OEBrightness = 1;
  digitalWrite(OE, LOW);  // Enable LED driver outputs.
  
  // Set the software SPI pins as outputs.
  // MOSI: D6 (GPIO12) and Clock: D5 (GPIO14)
  pinMode(D6, OUTPUT);
  pinMode(D5, OUTPUT);
  digitalWrite(D6, LOW);
  digitalWrite(D5, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  pwmCounter++;  // PWM counter cycles (0–255).
  
  // Read the ADC (potentiometer) every ~10 ms.
  if (currentMillis - previousMillisADC >= 10) {
    int sensorValue = analogRead(analogInPin);
    range = map(sensorValue, 0, 1023, 0, 6);
    previousMillisADC = currentMillis;
  }
        
  // Set LED modes based on the mapped ADC range.
  switch (range) {
    case 0:
      LEDA.setMode(LED_OFF);
      LEDB.setMode(LED_OFF);
      LEDC.setMode(LED_OFF);
      LEDD.setMode(LED_OFF);
      LEDE.setMode(LED_OFF);
      LEDF.setMode(LED_OFF);
      LEDG.setMode(LED_OFF);
      LEDH.setMode(LED_OFF);
      break;
    case 1:
      LEDA.setMode(LED_OFF);
      LEDB.setMode(LED_OFF);
      LEDC.setMode(LED_OFF);
      LEDD.setMode(LED_OFF);
      LEDE.setMode(SOLID_ON);
      LEDF.setMode(SOLID_ON);
      LEDG.setMode(SOLID_ON);
      LEDH.setMode(SOLID_ON);
      break;
    case 2:
      LEDA.setMode(LED_OFF);
      LEDB.setMode(LED_OFF);
      LEDC.setMode(LED_OFF);
      LEDD.setMode(LED_OFF);
      LEDE.setMode(CANDLE_ON);
      LEDF.setMode(CANDLE_ON);
      LEDG.setMode(CANDLE_ON);
      LEDH.setMode(CANDLE_ON);            
      break;
    case 3:
      LEDA.setMode(CANDLE_ON);
      LEDB.setMode(CANDLE_ON);
      LEDC.setMode(CANDLE_ON);
      LEDD.setMode(CANDLE_ON);
      LEDE.setMode(CANDLE_ON);
      LEDF.setMode(CANDLE_ON);
      LEDG.setMode(CANDLE_ON);
      LEDH.setMode(CANDLE_ON);  
      break;
    case 4:
      LEDA.setMode(CANDLE_ON);
      LEDB.setMode(CANDLE_ON);
      LEDC.setMode(CANDLE_ON);
      LEDD.setMode(CANDLE_ON);
      LEDE.setMode(SOLID_ON);
      LEDF.setMode(SOLID_ON);
      LEDG.setMode(SOLID_ON);
      LEDH.setMode(SOLID_ON);             
      break;
    case 5:
      LEDA.setMode(SOLID_ON);
      LEDB.setMode(SOLID_ON);
      LEDC.setMode(SOLID_ON);
      LEDD.setMode(SOLID_ON);
      LEDE.setMode(SOLID_ON);
      LEDF.setMode(SOLID_ON);
      LEDG.setMode(SOLID_ON);
      LEDH.setMode(SOLID_ON);                
      break;
    default:
      Serial.println("Error: Invalid Range");
      break;
  }

  // Update each LED channel.
  LEDA.Update();
  LEDB.Update();
  LEDC.Update();
  LEDD.Update();
  LEDE.Update();
  LEDF.Update();
  LEDG.Update();
  LEDH.Update();
  
  // Update the LED driver via software SPI:
  digitalWrite(latchPin, LOW);        // Begin latch.
  softSPITransfer(LEDOutputBuffer);     // Send the byte via our optimized software SPI.
  digitalWrite(latchPin, HIGH);       // Latch the data into the LED driver.
}

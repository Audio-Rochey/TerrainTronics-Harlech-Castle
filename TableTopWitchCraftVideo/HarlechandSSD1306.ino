//**************************************************************                //
//  Name    : Harlech and a SSD1306 OLED Display at the same time!              //
//  Author  : Dafydd Roche                                                      //
                                      //
//  Notes   : This code runs on the Wemos D1 Mini.                              //
//            This code does not use any of the Wifi                            //
//              Capabilities                                                    //
//            SSD1306 runs from the 3V3 rail and D1 and D2 for I2C              //
//            D5, D6, D8 are used for the SPI chain running into the LED driver.//
//            Output Enable is D4 and used (and PWM'd)                          //
//            This code example isn't the best code in the world. It just       //
//            about works. It uses a LOT of Adafruit code examples!             //
//                                                                              //
//            How the code works                                                //
//            Tickers are used for:                                             //
//            - Keepalive Circuit                                               //  
//            - Screen Invert Command                                           //
//            - Pattern Update/Step                                             //
//            - Flicker                                                         //
//********************************************************************************



#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!
#include <Wire.h> // Library used for I2C (to communicate withe little screen)
#include <Adafruit_GFX.h> // used to draw on the screen
#include <Adafruit_SSD1306.h> // used to draw on the screen
#include <Ticker.h>  //Ticker Library

const String softwareVersion = "V1.3";
const String softwareDate = "5/18/21";     

// Comment in/out which hardware you ahve.

#define harlechVer1p0
//#define harlechVer1p1

#ifdef harlechVer1p0
    const String harlechVer = "1p0";
    const int keepAlive = D3;// The Keepalive pin for USB Power Banks is D3.
    const String keepAlivePin = "D3";
    const int OE = D4; // Set the output enable pin. This can be rapidly PWM'd.
    const String OEpin = "D4";
#endif

#ifdef harlechVer1p1
    const String harlechVer = "1p1";
    const int keepAlive = D0;// The Keepalive pin for USB Power Banks is D0.
    const String keepAlivePin = "D0";
    const int OE = D7; // Was changed to D7 to allow D4 to be used for a status LED on the Wemos board. Used natively by wifi libraries etc.
    String OEpin = "D7";
#endif








// #define DEBUG 0 // this is used to switch debug comments on and off. Comment it out with // to optimize the code.

#define splash1_width 128
#define splash1_height 64







const uint8_t PROGMEM splash1_data[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1f, 0xff, 0xf0, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1f, 0xff, 0xf0, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x07, 0xff, 0xe0, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x18, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0xff, 0xfc, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0xff, 0xfe, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xff, 0xfe, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x3f, 0x7e, 0xf1, 0xe1, 0xc3, 0xc9, 0x9f, 0xbc, 0x38, 0x98, 0xf1, 0xe3, 0x80, 0x0f, 0x1c, 0x4c, 
0x0c, 0x60, 0xd9, 0xb2, 0x61, 0x8d, 0x86, 0x36, 0x6c, 0xd8, 0x63, 0x36, 0x00, 0x19, 0xb6, 0x7c, 
0x0c, 0x7c, 0xf1, 0xe3, 0xe1, 0x8d, 0x86, 0x3c, 0x6c, 0xd8, 0x63, 0x03, 0x80, 0x18, 0x36, 0x6c, 
0x0c, 0x60, 0xe1, 0xc2, 0x61, 0x8b, 0x86, 0x38, 0x6c, 0xb8, 0x63, 0x34, 0xc0, 0x19, 0xb6, 0x4c, 
0x0c, 0x7e, 0xd9, 0xb2, 0x63, 0xc9, 0x86, 0x36, 0x38, 0x98, 0xf1, 0xe3, 0x8c, 0x0f, 0x1c, 0x4c, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM ttwcraftlogo_data[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xc0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x40, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x1f, 0x83, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x02, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x03, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x02, 0x01, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xde, 0x02, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3c, 0x02, 0x01, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x3c, 0x02, 0x01, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x18, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x38, 0x37, 0x69, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3a, 0x5c, 0x97, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x3f, 0xd9, 0x15, 0xa3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x3f, 0x04, 0x0a, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x7e, 0x00, 0x01, 0xc9, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x7e, 0x00, 0x01, 0x3d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xca, 0x00, 0x01, 0x97, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x18, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1f, 0x00, 0x01, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1d, 0x00, 0x01, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0d, 0x00, 0x05, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00, 0x03, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x07, 0x00, 0x02, 0x01, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x56, 0x80, 0x0e, 0x63, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x57, 0x00, 0x14, 0x93, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xed, 0x53, 0x80, 0x1c, 0x83, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xe4, 0xe0, 0xc0, 0x08, 0x93, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xa6, 0xa0, 0x68, 0xb8, 0x66, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0xa0, 0x70, 0xf0, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x3e, 0xe0, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x80, 0x1f, 0xc0, 0x1c, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x01, 0xc1, 0xe0, 0x0c, 0x00, 0x38, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xe4, 0xf3, 0x41, 0xa8, 0x75, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 0xb9, 0xc8, 0xe0, 0xc1, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xde, 0x00, 0x0b, 0x9b, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x00, 0x0f, 0x7f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};





/* What the hell is a ticker?
 * Tickers are used to schedule things to happen in a repeatable manner.
 * For instance, to switch on the keepalive circuit for 1 second, it uses a ticker set for 1 second. When the 
 * ticker function is done, it calls a specific function.
 * The Display invert function gets called by the displayInvert ticker, inverts the screen, then sets the timer again.
 */
Ticker displayInvert; //used for the display timer.
Ticker USBBank;       // Used for the keepalive
Ticker FlickerA;      // Used to program flickers separated 
Ticker patternUpdate; // Used to step through the patterns.

/* How the Patterns are programmed
 *  Look below for an array titled: ledPattern[][3]. There are details there.
*/

/* How the Keepalive circuit works.
 * The keepalive circuit is connected to a transistor that switches about 
 * 80mA of current through a resistors to ground. It's used to trick a USB 
 * power bank into thinking a power hungry, charging phone is connected.
 * The pin on Harlech 1.0 is D3, on Harlech 1.1, it's 
*/
#define KeepAliveEnable = false; // change this to true if you want to enable the circuit.
int USBBankState = 0;


const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 9; // Time to switch the keep alive circuit off for.

/* How the Display Works
 *  The display is an SSD1306 that is programmed over an interface called I2C.
 *  Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
 *  On the Wemos D1 board, it's pins D1 and D2.
*/

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* Hardware Pin Definitions
 * These are find for Harlech 1p0 and 1p1.
 */
// These constants won't change. They're used to give names to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to.
const int latchPin = D8; //These definitions configure the pins for the SPI interface to talk to the LED driver
const int clockPin = D5;
const int dataPin = D6;

int outputValue = 0;        // value output to the PWM (analog out)
int dutyCycle = 1023;          // set starting - PWM Duty Cycle. Data is inverted so 1023 is off.


/* LED PATTERNS
 * The following array defines patterns. 
 * The first column sets which LED's are on or off. 
 * The second column defines the brightness. 0 is full on, 1023 is full off.
 * The third column defines how many miliseconds they should be on for.
 * The array can have many many steps, with short timing between them. Worst case, you'll run out of memory on the chip (160KBytes worth!).
 */
int ledPattern[][3] = {   {0b11111111,  0,  1000 } ,
                          {0b11111110,  0,  1000 } };


/*Set Up and initialize the pointers used for patterns */                        
int patternIndex = 0;
int patternData = 0;
int patternDelay = 0;
int numrows = 0;
int numRows = sizeof(ledPattern)/sizeof(ledPattern[0]);

bool displayinvertstate = false; //global variable for the state of displayinversion
long randNumber; /* This is all about setting up the flicker */
int flickerData = 0;
bool flickerState = false;


void setup() {




  
  randomSeed(analogRead(0)); // this is needed just to start the random number generator.
  WiFi.disconnect();     // We're not using Wifi, so lets disable it to save power.
  WiFi.forceSleepBegin();
  delay(1); //For some reason the modem won't go to sleep unless you do a delay  
  // initialize serial communications at 9600 bps: this is only used for communication with a PC.
  Serial.begin(9600);
  delay(100);




  

  Serial.println("*****");
  delay(100);
  Serial.println("TerrainTronics Demo Code For Tabletop Witchcraft");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date: ");
  Serial.print(softwareDate);
  Serial.print(" Harlech HW Version ");
  Serial.println(harlechVer);
  Serial.print("OE Pin: ");
  Serial.print(OEpin);
  Serial.print(" KA Pin: ");
  Serial.println(keepAlivePin);
  Serial.println("CH1 50/50 ON-OFF Every Second");
  Serial.println("CH2 CONSTANT ON");
  Serial.println("CH3 CONSTANT ON");
  Serial.println("CH4 CONSTANT ON");
  Serial.println("CH5-8 RANDOM FLICKER timed 500ms to 3000ms apart - 50mS off time.");

  
   
  pinMode(latchPin, OUTPUT); //set pins to output because they are addressed in the main loop
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!

  #ifdef KeepAliveEnable    // This special line looks to see if KeepAliveEnable has been "defined" -- look at the top of the code.
    Serial.println(F("KEEP ALIVE IS ENABLED"));
    USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.
  #endif
  

  // Make sure Screen is connected.
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  display.drawBitmap(0, 0, splash1_data, splash1_width, splash1_height, 1); // Shows the terrain tronics logo

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);             // Start at top-left corner
  display.println(softwareVersion);
   display.setCursor(82,0);             // Start at top-left corner
  display.println(softwareDate);

  
  display.display();        
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.display();
  display.drawBitmap(0, 0, ttwcraftlogo_data, splash1_width, splash1_height, 1); // Shows the tabletop witchcraft logo
  display.display();        
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();

  /* display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...*/


 

 terrainscreen();      // Draw characters of the default font
 displayInvert.attach(1, invertScreen);
 FlickerA.attach(1, Flicker);
 patternUpdate.attach(1, patternUpdater);
}

void loop() {


// Yup - nothing in the main loop. it's all run off timers (software tickers)

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

/*=======================================================================
// This is a subroutine that switches the keepAlive pin on/off to switch on a transistor that pulls current to ground, through a 47Ohm resistor
// PLEASE NOTE: You cannot leave this on for a long long time, as the resistor will begin to heat up beyond it's rating!*/

void keepAliveChangeState()
{
  // Serial.println("Keepalive Change");
  if (USBBankState == 0 ){
    Serial.println("Keepalive Switching off");
    USBBankState = 1;
    digitalWrite(keepAlive, 0); // Switch Transistor Off
    USBBank.attach(kaTimeOff, keepAliveChangeState); //Use attach_ms   
  }
  else if (USBBankState == 1 ){
    Serial.println("Keepalive Switching on");
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

void Flicker(){
  flickerState = !flickerState;
  if (flickerState == false){
    flickerData = 0b11111111;
    writetoHarlech(flickerData);
    randNumber = random(500, 3000);
    FlickerA.attach_ms(randNumber, Flicker); // set the timer again.
  }
  else {
    randNumber = random(1,15);
    //Serial.println(randNumber, BIN);
    randNumber = (randNumber << 4);
    //Serial.println(randNumber, BIN);
    flickerData = 0b00001111 | randNumber;
    writetoHarlech(flickerData);
    //Serial.println("flicker 50");
    FlickerA.attach_ms(50, Flicker); // set the timer again.
  }
  
  
  
}


void writetoHarlech(int flickerData) {
// Serial.println("WRITE TO SHIFTER");
   digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
   int txPatternData = patternData & flickerData;
   //Serial.println(flickerData);
   shiftOut(dataPin, clockPin, LSBFIRST, txPatternData);
   digitalWrite(latchPin, HIGH); // Tells the LED driver 
   analogWrite(OE, dutyCycle);
}

void patternUpdater(void) {
  patternIndex++; // Step to the next item in the index
    if (patternIndex == numRows) {    // if you hit the maximum number of rows in teh pattern, set it back to the 0'th row (the first)
      patternIndex = 0;
    }
  patternData = ledPattern[patternIndex][0];
  dutyCycle = ledPattern[patternIndex][1];
  patternDelay = ledPattern[patternIndex][2];

  writetoHarlech(flickerData); // write with the new pattern.
  patternUpdate.attach_ms(patternDelay, patternUpdater);
}

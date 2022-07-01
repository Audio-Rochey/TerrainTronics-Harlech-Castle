//**************************************************************//
//  Name    : Harlech Castle Wifi Control                       //
//  Author  : Dafydd Roche                                      //
//  Date    : 7/1/2022                                         //
//  Version : 1.1                                               //
//  Notes   : This code runs on the Wemos D1 Mini.              //
//****************************************************************

// Select only one of these.
//#define HarlechPG1p0 true
 #define HarlechPG1p1 true

const String softwareVersion = "V1.1";
const String softwareDate = "6/29/2022";     

#include "htmlfile.h"


/* Tap the little plus sign for details including users guide!
 *  
 * Hi. This code creates a server on your network once the wifi is configured.
 * When you first run it, look for a new Wifi network. It'll be called "TerrainTronicsNeedsWifi" with no password 
 * Configure it with your wifi details and it should connect to your network.
 * 
 * The functionality of the board is blocked for 60 seconds while the wifi configuration portal is open. 
 * 
 * If you need to reset that configuration, double press the reset button on the WemosD1 board.
 * 
 * 
 * Once you've configured the board to talk to your network, you'll access it from your browser, through your normal wifi connection. 
 * This code is set up so that you'll be able to find it at "http://terraintronics01" but that can be changed below. To know the IP address, 
 * use your PC to power the Wemos D1 mini and open the serial port monitor in arduino! 
 * You should be a message in there along the lines of: "*WM: [1] STA IP Address: 192.168.1.97"
 * If your network is being funny and not allowing the name, you can use http://192.168.1.97 (or whatever IP address your terrain was given) 
 * 
 * One good option might be to rename the "terraintronics01" to the name of your terrain, for instance "airstripterrain" - you'd then 
 * able to find it easily in your web browser.
 *  
 *  
 * How it works: 
 * https://github.com/tzapu/WiFiManager - WifiManager Library was used to allow users to config from their phone. it's cool.
 * Control is via WebSockets technology. The webpage is served FROM the Wemos. When you hit a button in the browser
 * your browser sends a "1", "2", or a "3" to the Wemos D1, which then takes an action based on which one you pressed.
 * The actions are detailed in the section starting with "void webSocketEvent" below.
 * The web page served by the wemos is below too. It is stored on the device. It's recommended to keep the web page as simple 
 * as possible to save space.
 * 
 * Using WebSockets allows the device to be driven from a number of different sources. A HTML page is nice and easy, however, 
 * using a websockets plugin, I've managed to use a Stream Deck to control LED's etc too.
 *
 *  
 * The hardware for this board is a standard harlech with the upper LED's replaced with resistors (to act as triggers 
 * for sound modules etc) and then the bottom 4 LED's populated. 
 * 
 * 
 * credits and notes:
 *  * Sketch: ESP8266_Part9_01_Websocket_LED
 * Intended to be run on an ESP8266
 * This Code Originally started over at: http://www.martyncurrey.com/esp8266-and-the-arduino-ide-part-9-websockets/
 * 
 * Double Reset Code is Awesome!: https://github.com/datacute/DoubleResetDetector/blob/master/examples/minimal/minimal.ino
 * 
 * 
 */



 


/* Double Reset is used to clear Wifi settings and start a new AP.*/


#include <DoubleResetDetector.h> //https://github.com/datacute/DoubleResetDetector/blob/master/examples/minimal/minimal.ino
// Number of seconds after reset during which a 
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);



#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

#include <strings_en.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager 










 
/*------------------------------------------------------------*/ 
/* Hardware Pin Definitions   - Includes revision specifics   */
/*------------------------------------------------------------*/
byte pin_led = D4;
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to.
int latchPin = D8;//These definitions configure the pins for the SPI interface to talk to the LED driver
int clockPin = D5;
int dataPin = D6;

#ifdef HarlechPG1p0
  const int keepAlive = D3;// The Keepalive pin for USB Power Banks is D3.
  const String keepAlivePin = "D3";
  int OE = D4; // Set the output enable pin. This can be rapidly PWM'd.
  const String HarlechVer = "1p0";
#endif
#ifdef HarlechPG1p1
  const int keepAlive = D0;
  const String keepAlivePin = "D0";
  int OE = D7; //it's D7 by default, but the board
  const String HarlechVer = "1p1";
#endif



/*------------------------------------------------------------*/ 
/* WIFI and Server Configuration                              */
/*------------------------------------------------------------*/
WiFiServer server(80);

WebSocketsServer webSocket = WebSocketsServer(81);
char myHostname[] = "TerrainTronics01"; // you can communicate with your product by accessing http://TerrainTronics01/


/*------------------------------------------------------------*/ 
/* USB Powerbank Keepalive Variables                          */
/*------------------------------------------------------------*/

// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library
Ticker USBBank; // Sets up a ticker for the USB Powerbank Keep Alive
Ticker PatternRun; // Used in the background task used to run the patterns
int USBBankState = 0;
// const int keepAlive = D3;// The Keepalive pin for USB Power Banks is D3 on Harlech.
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.


/*------------------------------------------------------------*/ 
/* LED Pattern Definitions and variables used in counting etc.*/
/*------------------------------------------------------------*/

// first group are the lights that should be on, second the PWM'd brightness and third, the amount of time between steps
// when using the board to trigger sound effects, 
int ledPattern[][3] = {   {0b11110000,  0,  500 } ,
                          {0b00000000,  0,  500 } ,
                          {0b11110000,  0,  500} ,
                          {0b00000000,  0,  500 } ,
                          {0b10000000,  0,  250 } ,
                          {0b01000000,  0,  250 } ,
                          {0b00100000,  0,  250 } ,
                          {0b00010000,  0,  250 } ,
                          {0b00000000,  0,  1000 } };


// Set Up and initialize the pointers used for patterns                          
int patternIndex = 0;
int patternData = 0;
int patternDelay = 0;
int numrows = 0;
int numRows = sizeof(ledPattern)/sizeof(ledPattern[0]);

int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output to the PWM (analog out)
int dutyCycle = 1023;          // set starting - PWM Duty Cycle. Data is inverted so 1023 is off.
int flashdelay = 500;
int breathDir = 0;  //Breathing Direction - 0 is counting up, 1 is counting down




//=======================================================================
// This is a subroutine that switches the keepAlive pin on/off to switch on a transistor that pulls current to ground, through a 47Ohm resistor
// PLEASE NOTE: You cannot leave this on for a long long time, as the resistor will begin to heat up beyond it's rating!

void keepAliveChangeState()
{
  Serial.println("Keepalive Change");
  if (USBBankState == 0 ){
    Serial.println("Switching off");
    USBBankState = 1;
    digitalWrite(keepAlive, 0); // Switch Transistor Off
    USBBank.attach(kaTimeOff, keepAliveChangeState); //Use attach_ms   
  }
  else if (USBBankState == 1 ){
    Serial.println("Switching on");
    USBBankState = 0;
    digitalWrite(keepAlive, 1); // Switch Transistor On
    USBBank.attach(kaTimeOn, keepAliveChangeState); //Use attach_ms   
  }
}

void runCurrentLedPattern() // this is a permanently running pattern.
{

  patternData = ledPattern[patternIndex][0];
  dutyCycle = ledPattern[patternIndex][1];
  patternDelay = ledPattern[patternIndex][2];
  //Serial.println(patternDelay);
  //Serial.println("runCurrentLedPattern");

   digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
   shiftOut(dataPin, clockPin, LSBFIRST, patternData);
   digitalWrite(latchPin, HIGH); // Tells the LED driver 
   analogWrite(OE, dutyCycle);
   patternIndex++; // Step to the next item in the index
   if (patternIndex == numRows) {    // if you hit the maximum number of rows in teh pattern, set it back to the 0'th row (the first)
    patternIndex = 0;
    }

    PatternRun.attach_ms(patternDelay, runCurrentLedPattern); //Use attach_ms   
  
}








WiFiManager wm;


int currentStaticHarlechOutputs = 0x00000000;

 
void setup()
{
  Serial.begin(115200); // Switches on the data back to the PC.
  Serial.println();
  Serial.println("Serial started at 115200");
  Serial.println();

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  
  //wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);

  WiFi.hostname(myHostname);

  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    wm.resetSettings();
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    Serial.println("No Double Reset Detected");
    digitalWrite(LED_BUILTIN, HIGH);
  }
  

  //reset settings - wipe credentials for testing
  //wm.resetSettings();
  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "TerrainTronicsNeedsWifi"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("TerrainTronicsNeedsWifi",""); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
      Serial.println("");
      Serial.println(F("[CONNECTED]"));   Serial.print("[IP ");  Serial.print(WiFi.localIP()); 
      Serial.println("]");
  }

  Serial.println("TerrainTronics Demo - Wifi Controlled Harlech!");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date of Compile ");
  Serial.print(softwareDate);
  Serial.print(" Harlech HW Version ");
  Serial.println(HarlechVer);
  
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led,LOW);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON and the board will get hot!

 
 
 
  // start a server
  server.begin();
  Serial.println("Server started");

  // Server Starts, then starts the USB Powerbank keepalive, and the first pattern. Use comments (//) to not start patterns until you click the GUI.
  USBBank.attach(5, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.
  // PatternRun.attach_ms(500, runCurrentLedPattern); // This starts the timer for the patterns.

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
 
void loop()
{
    wm.process(); // put to make the blociing work.
    //Serial.println("LOOP");
    webSocket.loop();
 
    WiFiClient client = server.available();     // Check if a client has connected
    if (!client)  {  return;  }
 
    client.flush();
    client.print( header );
    client.print( html_1 ); 
    Serial.println("New page served");
 
    delay(5);
}
 
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length)
{
  if(type == WStype_TEXT)
  {
      if (payload[0] == '0')
      {
          Serial.println("Pattern On");    
          runCurrentLedPattern();
          delay(5);
      }
      else if (payload[0] == '1')
      {
          
          PatternRun.attach_ms(0, NULL);
          Serial.println("All Lights On"); 
          digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
          currentStaticHarlechOutputs = 0b11110000;
          shiftOut(dataPin, clockPin, LSBFIRST, currentStaticHarlechOutputs);
          digitalWrite(latchPin, HIGH); // Tells the LED driver 
          analogWrite(OE, 0);
          delay(5);
      }
      else if (payload[0] == '2')
      {
          PatternRun.attach_ms(0, NULL);
          Serial.println("ALL LIGHTS OFF"); 
          digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
          currentStaticHarlechOutputs = 0b00000000;
          shiftOut(dataPin, clockPin, LSBFIRST, currentStaticHarlechOutputs);
          digitalWrite(latchPin, HIGH); // Tells the LED driver 
          analogWrite(OE, 0);
          delay(5);
      }
      else if (payload[0] == 't')
      {
          Serial.println("Trigger"); 
          digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
          shiftOut(dataPin, clockPin, LSBFIRST, (currentStaticHarlechOutputs | 0b00000001)); // bitwise OR to set the bits high.
          digitalWrite(latchPin, HIGH); // Tells the LED driver to latch
          delay (200); // add a 200mS delay.
          digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
          shiftOut(dataPin, clockPin, LSBFIRST, (currentStaticHarlechOutputs & 0b11111110)); // bitwise AND to set those pins back to currentStatic
          digitalWrite(latchPin, HIGH); // Tells the LED driver to latch
          analogWrite(OE, 0);
          delay(5);
      }
  }
 
  else 
  {
    Serial.print("WStype = ");   Serial.println(type);  
    Serial.print("WS payload = ");
    for(int i = 0; i < length; i++) { Serial.print((char) payload[i]); }
    Serial.println();
 
  }
}

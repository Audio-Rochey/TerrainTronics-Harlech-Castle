/**
Dafydd Roche - 8/16/2024

This work is heavily based on the work by Rui Santos over at randomnerdtutorials.
Yes, there are better and smarter ways to make a webserver that does all this cool stuff. :)

First time you download this, the device will set up it's own wifi access point that you need to connect to.
Change the wifi on your PC to find it. It will be called: TerrainTronicsConfigMePlease

Connect to it, open your browser and go to http://192.168.4.1 - there you can configure it to connect to your home wifi.

.\esptool.exe -b 460800 --port COM12 read_flash 0x00000 0x400000 Harlech-WebServerAndGUIONOFF.bin - the command line used to download an image of the flash.

//Updated code on 9.16 to auto reconnect to wifi.


*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/


const String softwareVersion = "V1.1";
const String softwareDate = "9/3/2024";     
const String HarlechVer = "1p1";

// Board specific stuff:
// The Harlech pins are defined as follows
//#define keepAlive D0 -- connected later.
int latchPin = D8; // Checked with PG1.1 of Harlech
int clockPin = D5; // Checked with PG1.1 of Harlech
int dataPin = D6; // Checked with PG1.1 of Harlech
int OE = D7; // Set the output enable pin. This can be rapidly PWM'd. Checked with PG1.1 of Harlech. (Soldered to lower 2)
int OEBrightness = 0;
int OEBrightnessIndex = 1;

#include <Ticker.h>  //Ticker Library
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;// The Keepalive pin for USB Power Banks is D0. UPdates in Harlech 1.1
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.


// Double Reset Detect basics
#ifdef ESP8266
  #define ESP8266_DRD_USE_RTC     true
#endif
#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector
// Number of seconds after reset during which a 
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 3
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0
//DoubleResetDetector* drd;




#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String D1LedoutState = "off";
String D2LedoutState = "off";
String D3LedoutState = "off";
String D4LedoutState = "off";
String D5LedoutState = "off";
String D6LedoutState = "off";
String D7LedoutState = "off";
String D8LedoutState = "off";

// Assign output variables to GPIO pins
//const int output5 = 5;
//const int output4 = 4;

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);


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

int ledWordToSend = 0x00;

void writeWordToHarlech()
{
  Serial.println(ledWordToSend);
  digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
  //shiftOut(dataPin, clockPin, LSBFIRST, 0xFF);
  shiftOut(dataPin, clockPin, LSBFIRST, ledWordToSend);
  digitalWrite(latchPin, HIGH); // Tells the LED driver 
  digitalWrite(OE, LOW); // Tells the LED driver 
}

void setup() {
  Serial.begin(115200);  
  while (!Serial)  // Wait for the serial connection to be establised.
  delay(500);


  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(OE, OUTPUT);
  //digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
  //shiftOut(dataPin, clockPin, LSBFIRST, 0xFF);
  //digitalWrite(latchPin, HIGH); // Tells the LED driver 
  //digitalWriteWrite(OE, HIGH); // Tells the LED driver

  Serial.println();
  delay(1000);
  Serial.println("*****");
  delay(100);
  Serial.println("TerrainTronics Demo - HTML Browser Control for Harlech ");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date of Compile ");
  Serial.print(softwareDate);
  Serial.print(" Harlech Hack HW Version ");
  Serial.println(HarlechVer);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
if (drd.detectDoubleReset()) 
  {
    Serial.println("");
    Serial.println("*********************");
    Serial.println("Double Reset Detected");
    Serial.println("*********************");
    wifiManager.resetSettings();
  } 
  else 
  {
    Serial.println("No Double Reset Detected");
  }

  
  wifiManager.autoConnect("TerrainTronicsConfigMePlease");

  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");


  server.begin();

  USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.
  
   if (!MDNS.begin("ttharlech")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);



}

void loop(){
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /1/on") >= 0) {
              Serial.println("D1Ledout on");
              D1LedoutState = "on";
              // N = N & ~ (1 << K) Do a bitwise OR
              ledWordToSend = ledWordToSend | 0b00000001;

            } else if (header.indexOf("GET /1/off") >= 0) {
              Serial.println("D1Ledout off");
              D1LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00000001);

            } else if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("D2Ledout on");
              D2LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b00000010;

            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("D2Ledout off");
              D2LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00000010);

            } else if (header.indexOf("GET /3/on") >= 0) {
              Serial.println("D3Ledout on");
              D3LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b00000100;

            } else if (header.indexOf("GET /3/off") >= 0) {
              Serial.println("D3Ledout off");
              D3LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00000100);

            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("D4Ledout on");
              D4LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b00001000;

            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("D4Ledout off");
              D4LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00001000);

            } else if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("D5Ledout on");
              D5LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b00010000;

            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("D5Ledout off");
              D5LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00010000);

            } else if (header.indexOf("GET /6/on") >= 0) {
              Serial.println("D6Ledout on");
              D6LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b00100000;

            } else if (header.indexOf("GET /6/off") >= 0) {
              Serial.println("D6Ledout off");
              D6LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b00100000);
              
            } else if (header.indexOf("GET /7/on") >= 0) {
              Serial.println("D7Ledout on");
              D7LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b01000000;

            } else if (header.indexOf("GET /7/off") >= 0) {
              Serial.println("D7Ledout off");
              D7LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b01000000);
              
            } else if (header.indexOf("GET /8/on") >= 0) {
              Serial.println("D8Ledout on");
              D8LedoutState = "on";
              ledWordToSend = ledWordToSend | 0b10000000;

            } else if (header.indexOf("GET /8/off") >= 0) {
              Serial.println("D8Ledout off");
              D8LedoutState = "off";
              ledWordToSend = ledWordToSend & ~(0b10000000);
              
            }
            writeWordToHarlech();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button1 {background-color: #b4b4b4;}");
            client.println(".button2 {background-color: #ffa500;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>TerrainTronics Web Server</h1>");
            client.println("<center><table>");
            client.println("<tr><td>");
            // Display current state, and ON/OFF buttons for D1Ledout  
            client.println("<p>D1Ledout</p>");
            // If the D1LedoutState is off, it displays the OFF button       
            if (D1LedoutState=="off") {
              client.println("<p><a href=\"/1/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/1/off\"><button class=\"button button2\">ON </button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for D2Ledout  
            client.println("<p>D2Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D2LedoutState=="off") {
              client.println("<p><a href=\"/2/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">ON </button></a></p>");
            }

            // Display current state, and ON/OFF buttons for D3Ledout  
            client.println("<p>D3Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D3LedoutState=="off") {
              client.println("<p><a href=\"/3/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/3/off\"><button class=\"button button2\">ON </button></a></p>");
            }

                        // Display current state, and ON/OFF buttons for D4Ledout  
            client.println("<p>D4Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D4LedoutState=="off") {
              client.println("<p><a href=\"/4/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">ON </button></a></p>");
            }
            client.println("</td>");
            client.println("<td>");
                             // Display current state, and ON/OFF buttons for D5Ledout  
            client.println("<p>D5Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D5LedoutState=="off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">ON </button></a></p>");
            }

                                         // Display current state, and ON/OFF buttons for D6Ledout  
            client.println("<p>D6Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D6LedoutState=="off") {
              client.println("<p><a href=\"/6/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/6/off\"><button class=\"button button2\">ON </button></a></p>");
            }

                                         // Display current state, and ON/OFF buttons for D7Ledout  
            client.println("<p>D7Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D7LedoutState=="off") {
              client.println("<p><a href=\"/7/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/7/off\"><button class=\"button button2\">ON </button></a></p>");
            }

                                         // Display current state, and ON/OFF buttons for D8Ledout  
            client.println("<p>D8Ledout</p>");
            // If the output4State is off, it displays the ON button       
            if (D8LedoutState=="off") {
              client.println("<p><a href=\"/8/on\"><button class=\"button button1\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/8/off\"><button class=\"button button2\">ON </button></a></p>");
            }
            client.println("</td></tr>");
            client.println("</table></Center>");



            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  drd.loop();
}

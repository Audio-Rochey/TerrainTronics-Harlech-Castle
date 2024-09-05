/*
* Wemos D1 Code with Harlech Board. Turns Harlech into an 8 output on/off LED driver with a PWM'd global output.
* Perfect for putting a few LED's around a single terrain piece.
*
*/



//https://github.com/garymueller/ESP-131GPIO
// Code heavily based on ^^^ as well as some of the work done by adafruit for object oriented programming.
/*
This code is based on Gary's above, mixed with some of hObject Oriented programming stuff from Adafruit to describe LED behaviour.
The code by itself doesn't work directly. You have to upload (using LittleFS) the html file for the configuration tool.
Instructions for doing so are below.
First time you power it up, connect to the espxxxx wifi point from your phone or pc and browse to 192.164.4.1 and enter your wifi details.
Then hit SAVE.



The code listens to 8 channels of data from Q Light Controller. (Universe and offset set by going to it's website)

To make this code work, you have to add the LittleFS files, and join the network then connect to 192.168.4.1

5/8/2024 - Notes and Specs on things to do:
- direct connect effects to be added: Thunder, Breathing, 50/50 Flashing (done).
- Harlech Model. 8ch On/Off (Threshold 127) with a single global brightness. 9 total DMX channels, first for global brightness control (0-255) then others On/Off (127 threshold) (DONE AND WORKING)
- Caernarfon - 3 Servo outputs, one DMX Channel Each. WS2812B support in up to 4 groups, with 3 DMX channels each. (total of 12 DMX channels.)



[Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32".

On macOS, press [⌘] + [Shift] + [P] to open the Command Palette in the Arduino IDE, then "Upload LittleFS to Pico/ESP8266/ESP32".
*/
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncE131.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncDNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>


// Double Reset Detector  -- There's a line used in setup too.
#define ESP8266_DRD_USE_RTC     true
#define DOUBLERESETDETECTOR_DEBUG       true  //false
#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector
// Number of seconds after reset during which a 
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 3
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0
DoubleResetDetector* drd;




using namespace std;

const String softwareVersion = "V1.0";
const String softwareDate = "5/9/2024";     
const String HarlechVer = "1p1";

#include <Ticker.h>  //Ticker Library
Ticker USBBank;
int USBBankState = 0;
const int keepAlive = D0;// The Keepalive pin for USB Power Banks is D0. UPdates in Harlech 1.1
const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.

int latchPin = D8; // Checked with PG1.1 of Harlech
int clockPin = D5; // Checked with PG1.1 of Harlech
int dataPin = D6; // Checked with PG1.1 of Harlech
int OE = D7; // Set the output enable pin. This can be rapidly PWM'd. Checked with PG1.1 of Harlech. (Soldered to lower 2)
int OEBrightness = 0;
int OEBrightnessIndex = 1;



// ***** USER SETUP STUFF *****
String ssid = "test"; // replace with your SSID.
String password = "testtest"; // replace with your password.

String CONFIG_FILE = "Config.json";
DynamicJsonDocument Config(1024);
AsyncWebServer server(80);
AsyncDNSServer dnsServer;
ESPAsyncE131 e131;

//for Wemos D1 R1 pins are 16,5,4,14,12,13,0,2
//vector<int> GpioVector{16,5,4,14,12,13,0,2};
//for Wemos D1 R2 pins are 16,5,4,0,2,14,12,13
vector<int> GpioVector{16,5,4,0,2,14,12,13};
int DigitalOn = HIGH;
int DigitalOff = LOW;

int DMXValue[24]; // 24 values are captured including 0.

//Forward Declarations
String WebReplace(const String& var);

bool LoadConfig();
void SaveConfig(AsyncWebServerRequest* request);

void InitGpio();
void InitWifi();
void Init131();
void InitWeb();
void E131Handler(); //Custom algo that reads the latest package and updates the Globals.

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










/*
 * Sets up the initial state and starts sockets
 */
void setup() {
  delay(3000);
  Serial.begin(115200);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(500);
  
  Serial.println();
  delay(1000);
  Serial.println("*****");
  delay(100);
  Serial.println("TerrainTronics Demo - DMX Reciever ");
  Serial.print("Version ");
  Serial.print(softwareVersion);
  Serial.print(" Date of Compile ");
  Serial.print(softwareDate);
  Serial.print(" Harlech Hack HW Version ");
  Serial.println(HarlechVer);
  
  // Here's the double reset code
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset()) 
  {
    Serial.println("Double Reset Detected - Configuring LittleFS and deleting config.json");
    if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    }
    LittleFS.remove(CONFIG_FILE);
    Serial.println("Config File Deleted");

  } 

  

  USBBank.attach(1, keepAliveChangeState); // This starts the timer for the USB Power Bank Keepalive.
  
  Serial.println("==============================");
  Serial.println("Start Setup");
  
  LoadConfig(); // Tries to open Config.json, if it doesn't find it. It may have been deleted earlier!

  //InitGpio();
  pinMode(keepAlive, OUTPUT);     // KeepAlive pin needs pulling low, otherwise it'll float up
  digitalWrite(keepAlive, LOW);   // and switch on the transistor. - THIS IS IMPORTANT, OTHERWISE IT FLOATS UP AND SWITCHES THE TRANSISTOR ON!
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  InitWifi();

  Init131();

  InitWeb();

  Serial.println("Completed setup. Starting loop");
  Serial.println("==============================");
}

/*
 * Main Event Loop
 */
void loop() {
  drd->loop();
  e131_packet_t packet;
  int ledWordToSend = 0x00;


  while(!e131.isEmpty()) {
    E131Handler();
  }
  if (DMXValue[1] > 127){
    ledWordToSend = ledWordToSend | 0b00000001;
  }
  if (DMXValue[2] > 127){
    ledWordToSend = ledWordToSend | 0b00000010;
  }
  if (DMXValue[3] > 127){
    ledWordToSend = ledWordToSend | 0b00000100;
  }
    if (DMXValue[4] > 127){
    ledWordToSend = ledWordToSend | 0b00001000;
  }
    if (DMXValue[5] > 127){
    ledWordToSend = ledWordToSend | 0b00010000;
  }
  if (DMXValue[6] > 127){
    ledWordToSend = ledWordToSend | 0b00100000;
  }
  if (DMXValue[7] > 127){
    ledWordToSend = ledWordToSend | 0b01000000;
  }
    if (DMXValue[8] > 127){
    ledWordToSend = ledWordToSend | 0b10000000;
  }

    digitalWrite(latchPin, LOW); // Tells the LED driver IC to listen
    shiftOut(dataPin, clockPin, LSBFIRST, ledWordToSend);
    digitalWrite(latchPin, HIGH); // Tells the LED driver 
   analogWrite(OE, (255-DMXValue[0]));

}// end void loop 




/*
 * E131Handler
 */
void E131Handler()
{
  e131_packet_t packet;
  bool Digital = Config["GPIO"]["digital"].as<bool>();
  int DigitalThreshold = Config["GPIO"]["digital_threshold"].as<int>();
  int ChannelOffset = Config["E131"]["channel_offset"].as<int>();
  e131.pull(&packet);     // Pull packet from ring buffer
    uint16_t num_channels = htons(packet.property_value_count) - 1;
    
     for(int GpioIndex = 0, ChannelIndex = ChannelOffset;
         GpioIndex < 16 && ChannelIndex < num_channels; 
         ++GpioIndex, ++ChannelIndex) {


      uint16_t data = packet.property_values[ChannelIndex+1];
      DMXValue[GpioIndex] = data;
      //Serial.printf("%d:%d; ",GpioIndex,(DMXValue[GpioIndex]));

    }//for loop
    //Serial.println("");
}


/*
 * InitGpio
 */
void InitGpio()
{
  //initialize GPIO pins
  for(int i = 0; i < GpioVector.size(); ++i)  {
    pinMode(GpioVector[i], INPUT);
    //digitalWrite(GpioVector[i], DigitalOff);
  }
}

/*
 * Loads the config from SPPS into the Config variable
 */
bool LoadConfig()
{
  // Initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return false;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    //First Run, setup defaults
    Config["network"]["hostname"] = "TT-" + String(ESP.getChipId(), HEX);
    Config["network"]["ssid"] = ssid;
    Config["network"]["password"] = password;
    Config["network"]["static"] = false;
    Config["network"]["static_ip"] = "192.168.1.100";
    Config["network"]["static_netmask"] = "255.255.255.0";
    Config["network"]["static_gateway"] = "192.168.1.1";
    Config["network"]["access_point"] = true;
    
    Config["E131"]["multicast"] = "true";
    Config["E131"]["universe"] = 1;
    Config["E131"]["channel_offset"] = 0;

    Config["GPIO"]["digital"] = "true";
    Config["GPIO"]["digital_threshold"] = 127;
    Config["GPIO"]["digital_lowlevel"] = false;
    
  } else {
    Serial.println("Loading Configuration File");
    deserializeJson(Config, file);
  }

  if(Config["GPIO"]["digital_lowlevel"].as<bool>()) {
    DigitalOn = LOW;
    DigitalOff = HIGH;
  } else {
    DigitalOn = HIGH;
    DigitalOff = LOW;
  }

  serializeJson(Config, Serial);Serial.println();
  return true;
}

void SaveConfig(AsyncWebServerRequest* request)
{
  Serial.println("Saving Configuration File");
  
  //debug
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    Serial.printf("Param: %s, %s\n", p->name().c_str(), p->value().c_str());
  }

  //Validate Config

  Config["network"]["hostname"] = request->getParam("hostname",true)->value();
  Config["network"]["ssid"] = request->getParam("ssid",true)->value();
  Config["network"]["password"] = request->getParam("password",true)->value();
  //checkbox status isnt always included if toggled off
  Config["network"]["static"] = 
    (request->hasParam("static",true) && (request->getParam("static",true)->value() == "on"));
  Config["network"]["static_ip"] = request->getParam("static_ip",true)->value();
  Config["network"]["static_netmask"] = request->getParam("static_netmask",true)->value();
  Config["network"]["static_gateway"] = request->getParam("static_gateway",true)->value();
  Config["network"]["access_point"] = 
    (request->hasParam("access_point",true) && (request->getParam("access_point",true)->value() == "on"));
  
  //checkbox status isnt always included if toggled off
  Config["E131"]["multicast"] = 
    (request->hasParam("multicast",true) && (request->getParam("multicast",true)->value() == "on"));
  Config["E131"]["universe"] = request->getParam("universe",true)->value();
  Config["E131"]["channel_offset"] = request->getParam("channel_offset",true)->value();

  //checkbox status isnt always included if toggled off
  Config["GPIO"]["digital"] =
    (request->hasParam("digital",true) && (request->getParam("digital",true)->value() == "on"));
  Config["GPIO"]["digital_threshold"] = request->getParam("digital_threshold",true)->value();
  Config["GPIO"]["digital_lowlevel"] =
    (request->hasParam("digital_lowlevel",true) && (request->getParam("digital_lowlevel",true)->value() == "on"));

  File file = LittleFS.open(CONFIG_FILE, "w");
  if(file) {
    serializeJson(Config, file);
    serializeJson(Config, Serial);
  }

  request->send(200);
  ESP.restart();
}

/*
 * Initialiaze Wifi (DHCP/STATIC and Access Point)
 */
void InitWifi()
{
  // Switch to station mode and disconnect just in case
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  WiFi.hostname(Config["network"]["hostname"].as<const char*>());

  //configure Static/DHCP
  if(Config["network"]["static"].as<bool>()) {
    IPAddress IP; IP.fromString(Config["network"]["static_ip"].as<String>());
    IPAddress Netmask; Netmask.fromString(Config["network"]["static_netmask"].as<String>());
    IPAddress Gateway; Gateway.fromString(Config["network"]["static_gateway"].as<String>());

    if(WiFi.config(IP, Netmask, Gateway)) {
      Serial.println("Successfully configured static IP");
    } else {
      Serial.println("Failed to configure static IP");
    }
  } else {
    Serial.println("Connecting with DHCP");
  }

  //Connect
  int Timeout = 15000;
  WiFi.begin(Config["network"]["ssid"].as<String>(), Config["network"]["password"].as<String>());
  if(WiFi.waitForConnectResult(Timeout) != WL_CONNECTED) {
    if(Config["network"]["access_point"].as<bool>()) {
      Serial.println("*** FAILED TO ASSOCIATE WITH AP, GOING SOFTAP ***");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(Config["network"]["hostname"].as<String>());
      dnsServer.start(53, "*", WiFi.softAPIP());
      String netname = (Config["network"]["hostname"]);
      Serial.println("");
      Serial.println("");
      Serial.println("*************");
      Serial.print("Connect to "); // telling the user how to find the access point
      Serial.println(netname);
      Serial.println("Open a browser to configure me at 192.168.4.1"); // telling the user to go to 192.168.4.1
      Serial.println("*************");
    } else {
      Serial.println(F("*** FAILED TO ASSOCIATE WITH AP, REBOOTING ***"));
      ESP.restart();
    }
  } else {
    Serial.printf("Connected as %s\n",WiFi.localIP().toString().c_str());
  }

  WiFi.printDiag(Serial);
}

void Init131()
{
  if(Config["E131"]["multicast"].as<bool>()) {
    Serial.printf("Initializing Multicast with universe <%d>\n",Config["E131"]["universe"].as<int>());
    e131.begin(E131_MULTICAST, Config["E131"]["universe"]);
  } else {
    Serial.println("Initializing Unicast");
    e131.begin(E131_UNICAST);
  }
}


/*
 * Initializes the webserver
 */
void InitWeb()
{
  //enables redirect to /index.html on AP connection
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, WebReplace);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, WebReplace);
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.png", "image/png");
  });
  server.on("/SaveConfig", HTTP_POST, [](AsyncWebServerRequest *request){
    SaveConfig(request);
  });
  server.on("/SetRelay", HTTP_GET, [](AsyncWebServerRequest *request){
    int relay = request->getParam("relay")->value().toInt();
    if(relay<0 || relay >= GpioVector.size()) {
      Serial.println("SetRelay - Index out of range");
      return;
    }
    digitalWrite(GpioVector[relay], (request->getParam("checked")->value() == "true") ? DigitalOn : DigitalOff);
    request->send(200);
  });

  server.begin();
}

/*
 * WebReplace:
 * Substitutes variables inside of the html
 */
String WebReplace(const String& var)
{
  //Status Page
  if (var == "SSID") {
    return (String)WiFi.SSID();
  } else if (var == "HOSTNAME") {
    return (String)WiFi.hostname();
  } else if (var == "IP") {  
    if(WiFi.getMode() == WIFI_AP) {
      return WiFi.softAPIP().toString();
    } else {
      return WiFi.localIP().toString();
    }
  } else if (var == "MAC") {
    return (String)WiFi.macAddress();
  } else if (var == "RSSI") {
    return (String)WiFi.RSSI();
  } else if (var == "HEAP") {
    return (String)ESP.getFreeHeap();
  } else if (var == "UPTIME") {
    return String(millis());
  } else if (var == "UNIVERSE") {
    return Config["E131"]["universe"];
  } else if (var == "PACKETS") {
    return (String)e131.stats.num_packets;
  } else if (var == "PACKET_ERR") {
    return (String)e131.stats.packet_errors;
  } else if (var == "LAST_IP") {
    return e131.stats.last_clientIP.toString();

  //Configuration Page   
  } else if (var == "CONFIG_HOSTNAME") {
    return Config["network"]["hostname"];
  } else if (var == "CONFIG_SSID") {
    return Config["network"]["ssid"];
  } else if (var == "CONFIG_PASSWORD") {
    return Config["network"]["password"];
  } else if (var == "CONFIG_AP") {
    if(Config["network"]["access_point"].as<bool>())
      return "checked";
     else
      return "";
  } else if (var == "CONFIG_STATIC") {
    if(Config["network"]["static"].as<bool>())
      return "checked";
     else
      return "";
  } else if (var == "CONFIG_STATIC_IP") {
    return Config["network"]["static_ip"];
  } else if (var == "CONFIG_STATIC_NETMASK") {
    return Config["network"]["static_netmask"];
  } else if (var == "CONFIG_STATIC_GATEWAY") {
    return Config["network"]["static_gateway"];
  } else if (var == "CONFIG_MULTICAST") {
    if(Config["E131"]["multicast"].as<bool>())
      return "checked";
     else
      return "";
  } else if (var == "CONFIG_UNIVERSE") {
    return Config["E131"]["universe"];
  } else if (var == "CONFIG_CHANNEL_OFFSET") {
    return Config["E131"]["channel_offset"];
  } else if (var == "CONFIG_DIGITAL") {
    if(Config["GPIO"]["digital"].as<bool>())
      return "checked";
     else
      return "";
  } else if (var == "CONFIG_THRESHOLD") {
    return Config["GPIO"]["digital_threshold"];
  } else if (var == "CONFIG_LOWLEVEL") {
    if(Config["GPIO"]["digital_lowlevel"].as<bool>())
      return "checked";
     else
      return "";

  //Relay Page
  } else if (var == "RELAYS") {
    String Relays = "";
    for(int i = 0; i < GpioVector.size(); ++i) {
      Relays += "<label>Relay "+String(i+1)+" ("+GpioVector[i]+")</label>";
      Relays += "  <label class=\"switch\">";
      Relays += "  <input type=\"checkbox\" ";
      if(digitalRead(GpioVector[i]) == DigitalOn) {
        Relays += "checked";
      }
      Relays += " onclick=\"fetch('SetRelay?relay="+String(i)+"&checked='+this.checked);\">";
      Relays += "  <span class=\"slider round\"></span>";
      Relays += "</label>";
      Relays += "<br><br>";
    }
    return Relays;
  }
 
  return var;
}

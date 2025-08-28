/*
======================================================================
  TerrainTronics — Harlech Castle LED Controller (ESP8266 / Wemos D1 mini)
  Firmware: Harlech_Milestone_E_Cleaned_v17_patched.ino
======================================================================
*/
// Overview
/*
Overview
  • WiFi-configurable LED controller for model/miniature terrain lighting
  • Features: WiFiManager portal (root /), LED config (/led), Dynamic Scenes (/dynled),
    EEPROM persistence, SSDP discovery, optional mDNS, and WebSocket scene switching

Where to Buy the Harlech Castle Board
  • Etsy  : https://www.etsy.com/shop/TerrainTronics
  • Tindie: https://www.tindie.com/stores/terraintronics/

Recommended ESP8266 Module (Wemos D1 mini)
  • Amazon search (choose LOLIN/WeMos originals or quality clones):
    https://www.amazon.com/s?k=wemos+d1+mini+esp8266

TerrainTronics on YouTube
  • Tutorials, build logs, and demos: https://www.youtube.com/results?search_query=TerrainTronics

Notes
  • For LAN discovery on Windows, SSDP provides a "Network" entry (description.xml)
  • For local hostnames (http://<hostname>.local), ensure mDNS is supported on your OS
  • Dynamic scenes can be switched from a phone (web) or automation (WebSocket)

======================================================================
*/

// Feature checklist (running notes)
/* 
- Add Configure LED Button to home screen - DONE
- Copy CSS to the LED Page. - looks good enough! - DONE
- Add somewhere to rewrite the hostname - DONE
- Change the Title of the WiFi Manager - DONE
- Dark Mode - DONE
- Websocket Support Added and working with Streamdeck.
- Dynamic Scenes page added.
*/

// WebSocket Quick Guide
/*
- Connect: ws://<device-ip>:82/  (or ws://<hostname>.local:82/)
- Send: '0' → Scene A, '1' → Scene B, '2' → Scene C
- Also valid: 'apply:0' / 'apply:1' / 'apply:2'
- Device broadcast on change: {event:active, profile:N}
- Test in a desktop browser console:
    const W = new WebSocket('ws://<IP>:82/');
    W.onmessage = e => console.log('MSG:', e.data);
    W.onopen = () => W.send('0'); // apply Scene A
Notes
  • Scenes are created/edited on /dynled; WebSocket only applies.
  • Applying via the website also broadcasts the same 'active' event.
  • Ensure controller and device are on the same LAN; allow port 82 through firewalls.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266SSDP.h>
#include <vector>
#include <WebSocketsServer.h>

// ---- Forward declarations (fix compile order dependencies) ----
static String macNoColons();
static String stableUUID();

// ---------------- User-configurable knobs ----------------
static const bool OUTPUT_ACTIVE_LOW = false;
static const int  CONFIG_LED_PIN        = LED_BUILTIN;   // D4 onboard
static const bool CONFIG_LED_ACTIVE_LOW = true;          // onboard LED is active-LOW
static const uint16_t CONFIG_LED_PERIOD = 250;           // ms

static const uint8_t  PWM_STEPS          = 16;
static const uint16_t PWM_REFRESH_HZ     = 500;
static const uint32_t PWM_TICK_US        = 1000000UL / (PWM_REFRESH_HZ * PWM_STEPS);
static const uint8_t  CANDLE_MIN_STEPS   = 1;
static const int8_t   CANDLE_LEVEL_BIAS  = 0;
// Candle timing knobs (ms)
static const uint16_t CANDLE_START_MIN_MS = 150;  // first schedule after power-up
static const uint16_t CANDLE_START_MAX_MS = 400;
static const uint16_t CANDLE_TARGET_MIN_MS = 50; // subsequent retarget interval
static const uint16_t CANDLE_TARGET_MAX_MS = 150;
static const char*    PORTAL_TITLE       = "Harlech Controller";  // Shown in WiFiManager & pages
static const bool     WM_DARK_MODE      = true;               // Dark theme for WiFiManager portal

// ---------------- EEPROM layout ----------------
static const uint16_t EEPROM_SIZE  = 128;
static const uint8_t  EEPROM_SIG   = 0x5A;
static const uint8_t  EE_SIG       = 0;
static const uint8_t  EE_MODES     = 1;            // 8 bytes: LED modes
static const uint8_t  EE_HOSTLEN   = 9;            // 1 byte: hostname length (0 = unset)
static const uint8_t  EE_HOSTSTR   = 10;           // up to 31 bytes: hostname
static const uint8_t  HOST_MAXLEN  = 31;           // keep short; mDNS label limit is 63

// Scene storage (3 profiles x 8 bytes) + active index
static const uint8_t EE_PROF0 = EE_HOSTSTR + HOST_MAXLEN; // 10 + 31 = 41
static const uint8_t EE_PROF1 = EE_PROF0 + 8;             // 49
static const uint8_t EE_PROF2 = EE_PROF1 + 8;             // 57
static const uint8_t EE_ACTIVE = EE_PROF2 + 8;            // 65

// ---------------- WiFi / Server ----------------
WiFiManager wm;
static ESP8266WebServer* g_server = nullptr;  // use WiFiManager's internal server
WebSocketsServer ws(82);  // WebSocket server for scene switching

// ---------------- Fast GPIO for 74HC595 ----------------
extern "C" {
  #include "eagle_soc.h"
}
#define GPIO_SET(gpio) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1U << (gpio)))
#define GPIO_CLR(gpio) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (1U << (gpio)))
static const uint8_t GPIO_DATA  = 12; // D6 -> GPIO12 (SER)
static const uint8_t GPIO_CLOCK = 14; // D5 -> GPIO14 (SRCLK)
static const uint8_t GPIO_LATCH = 15; // D8 -> GPIO15 (RCLK)  (⚠ strap pin)
static const uint8_t GPIO_OE    = 13; // D7 -> GPIO13 (OE, active LOW)
inline void pinHigh(uint8_t g){ GPIO_SET(g); }
inline void pinLow (uint8_t g){ GPIO_CLR(g); }

// ---------------- LED Modes / State ----------------
enum LedMode : uint8_t { LED_OFF=0, LED_ON=1, LED_CANDLE=2 };
volatile LedMode ledMode[8] = {LED_OFF,LED_OFF,LED_OFF,LED_OFF,LED_OFF,LED_OFF,LED_OFF,LED_OFF};
uint8_t outputByte = 0;

struct FlickerState{ uint8_t level; uint8_t target; uint32_t nextTarget; } flick[8];
uint32_t lastPwmTick = 0;
uint8_t  pwmStep     = 0;
uint8_t  duty[8]     = {0,0,0,0,0,0,0,0};

// ----- Health logging knobs/state -----
static const uint32_t HEALTH_PERIOD_MS = 180000; // every ~3 minutes (tweak to taste)
static uint32_t lastHealthLog = 0;
volatile uint16_t wsClientCount = 0; // updated via WebSocket events
static String gUUID; // persistent UUID string used by SSDP and description.xml

// ---------------- Utilities ----------------
static inline uint8_t gamma2(uint8_t x){
  uint16_t y = (uint16_t)x * (uint16_t)x / 255;
  if(y>255) y=255;
  return (uint8_t)y;
}

static void srWriteByte_fast(uint8_t value){
  pinLow(GPIO_LATCH);
  for(int8_t i=7;i>=0;--i){
    pinLow(GPIO_CLOCK);
    if(value & (1<<i)) pinHigh(GPIO_DATA); else pinLow(GPIO_DATA);
    pinHigh(GPIO_CLOCK);
  }
  pinLow(GPIO_DATA);
  pinHigh(GPIO_LATCH);
}

static inline void setOutputBit(uint8_t idx, bool on){
  if(idx>7) return;
  bool bitLevel = on;
  if(OUTPUT_ACTIVE_LOW) bitLevel = !on;
  if(bitLevel) outputByte |= (1<<idx); else outputByte &= ~(1<<idx);
}

// ---------------- Candle Flicker Engine ----------------
static void initFlicker(){
  for(uint8_t i=0;i<8;i++){
    flick[i].level      = random(120,200);
    flick[i].target     = random(80,220);
    flick[i].nextTarget = millis() + (uint32_t)random(CANDLE_START_MIN_MS, CANDLE_START_MAX_MS);
    duty[i]             = gamma2(flick[i].level);
  }
}

static void updateFlicker(){
  uint32_t now = millis();
  for(uint8_t i=0;i<8;i++){
    if(ledMode[i] != LED_CANDLE) continue;

    if(now >= flick[i].nextTarget){
      int16_t nt = (int16_t)flick[i].target + (int8_t)random(-35, 36);
      if(nt < 0) nt = 0; else if(nt > 255) nt = 255;
      flick[i].target = (uint8_t)nt;
      flick[i].nextTarget = now + (uint32_t)random(CANDLE_TARGET_MIN_MS, CANDLE_TARGET_MAX_MS);
    }

    int16_t delta = (int16_t)flick[i].target - (int16_t)flick[i].level;
    flick[i].level += (int8_t)(delta >> 2);
    int16_t j = (int16_t)flick[i].level + (int8_t)random(-1, 2);
    if(j<0) j=0; else if(j>255) j=255;
    flick[i].level = (uint8_t)j;
    int16_t biased = (int16_t)flick[i].level + CANDLE_LEVEL_BIAS;
    if(biased<0) biased=0; else if(biased>255) biased=255;
    duty[i] = gamma2((uint8_t)biased);
  }
}

// ---------------- PWM Frame ----------------
static void updatePWM(){
  uint32_t nowUs = micros();
  if(nowUs - lastPwmTick < PWM_TICK_US) return;
  lastPwmTick = nowUs;

  uint8_t b = 0;
  for(uint8_t i=0;i<8;i++){
    bool on = (ledMode[i] == LED_ON);
    if(ledMode[i] == LED_CANDLE){
      uint8_t threshold = (uint16_t)duty[i] * PWM_STEPS / 255;
      if(threshold < CANDLE_MIN_STEPS) threshold = CANDLE_MIN_STEPS;
      on = (pwmStep < threshold);
    }
    if(on) b |= (1<<i);
  }
  uint8_t out = OUTPUT_ACTIVE_LOW ? (~b) : b;
  srWriteByte_fast(out);
  pwmStep = (pwmStep + 1) % PWM_STEPS;
}

// ---------------- HTML helpers ----------------
static String optionFor(uint8_t m){
  LedMode mm = (LedMode)m;
  String s; s.reserve(120);
  s += F("<option value='0'"); if(mm==LED_OFF)    s += F(" selected"); s += F(">Off</option>");
  s += F("<option value='1'"); if(mm==LED_ON)     s += F(" selected"); s += F(">On</option>");
  s += F("<option value='2'"); if(mm==LED_CANDLE) s += F(" selected"); s += F(">Candle</option>");
  return s;
}

static void handleRoot(){
  if(!g_server) return;
  String html; html.reserve(4096);
  html += F("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>");
  html += "<title>" + String(PORTAL_TITLE) + " • LED Configuration</title>";
  // WiFiManager-like styling (mirrors the portal look & feel)
  html += F("<style>");
  html += F("*{box-sizing:border-box}html,body{margin:0;padding:0;font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;background:#222;color:#fff} ");
  html += F(".wrap{display:flex;justify-content:center;padding:24px}.container{width:100%;max-width:520px} h2{margin:0 0 8px 0;font-weight:600} h3{margin:0 0 16px 0;font-weight:500;color:#bbb}");
  html += F(".card{background:#111;border:1px solid #333;border-radius:8px;padding:16px;box-shadow:0 1px 0 rgba(0,0,0,.4)} ");
  html += F(".button{display:inline-block;border:0;border-radius:4px;background:#1fa3ec;color:#fff;padding:10px 16px;text-decoration:none;cursor:pointer} .button:hover{background:#0f78a0} ");
  html += F("table{width:100%;border-collapse:collapse;margin:8px 0} td{padding:8px 6px;vertical-align:middle} td:first-child{opacity:.9} ");
  html += F("select{width:100%;padding:8px 10px;border:1px solid #444;border-radius:6px;background:#181818;color:#fff;appearance:none} ");
  html += F(".row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px} .row .button{flex:1 1 180px;text-align:center} .subtitle{font-size:12px;color:#aaa;margin-top:6px}");
  html += F("</style></head><body>");

  html += F("<div class='wrap'><div class='container'>");
  html += F("<div class='card'>");
  html += "<h2>" + String(PORTAL_TITLE) + "</h2><h3>LED Configuration</h3>";
  html += F("<form method='POST' action='/led/save'><table>");
  for(uint8_t i=0;i<8;i++){
    html += "<tr><td>LED" + String(i) + "</td><td><select name='m" + String(i) + "'>";
    html += optionFor(ledMode[i]);
    html += F("</select></td></tr>");
  }
  html += F("</table><div class='row'>");
  html += F("<button class='button' type='submit'>Save</button>");
  html += F("<a class='button' href='/'>Back</a>");
  html += F("</div><div class='subtitle'>Changes are stored to EEPROM.</div>");
  html += F("</form></div></div></div>");

  html += F("</body></html>");
  g_server->send(200,"text/html",html);
}


static void handleSave(){
  if(!g_server) return;
  for(uint8_t i=0;i<8;i++){
    String key = "m" + String(i);
    if(g_server->hasArg(key)){
      int v = g_server->arg(key).toInt();
      if(v<0) v=0; if(v>2) v=2;
      ledMode[i] = (LedMode)v;
    }
  }
  EEPROM.write(EE_SIG,EEPROM_SIG);
  for(uint8_t i=0;i<8;i++) EEPROM.write(EE_MODES+i,(uint8_t)ledMode[i]);
  EEPROM.commit();
  g_server->sendHeader("Location","/led");
  g_server->send(303);
}

// ------------- Hostname helpers & UI -------------
static String sanitizeHostname(const String& in){
  String out; out.reserve(HOST_MAXLEN);
  for(size_t i=0;i<in.length() && out.length()<HOST_MAXLEN;i++){
    char c = in[i];
    if((c>='a'&&c<='z')||(c>='0'&&c<='9')||(c=='-')) out+=c;
    else if(c>='A'&&c<='Z') out+=(char)(c+32);
  }
  while(out.length() && !((out[0]>='a'&&out[0]<='z')||(out[0]>='0'&&out[0]<='9'))) out.remove(0,1);
  while(out.length() && !((out[out.length()-1]>='a'&&out[out.length()-1]<='z')||(out[out.length()-1]>='0'&&out[out.length()-1]<='9'))) out.remove(out.length()-1,1);
  return out;
}
static String defaultHostname(){
  String mac = macNoColons();
  String tail = mac.substring(max(0,(int)mac.length()-6));
  return String("Harlech-") + tail;
}
static void saveHostnameToEEPROM(const String& h){
  uint8_t len = (uint8_t)min((int)HOST_MAXLEN, (int)h.length());
  EEPROM.write(EE_HOSTLEN, len);
  for(uint8_t i=0;i<HOST_MAXLEN;i++){
    uint8_t v = (i<len)? (uint8_t)h[i] : 0;
    EEPROM.write(EE_HOSTSTR+i, v);
  }
  EEPROM.commit();
}
static String loadHostnameFromEEPROM(){
  uint8_t len = EEPROM.read(EE_HOSTLEN);
  if(len==0 || len>HOST_MAXLEN) return String();
  String h; h.reserve(len);
  for(uint8_t i=0;i<len;i++) h += (char)EEPROM.read(EE_HOSTSTR+i);
  return h;
}
static void handleHost(){
  if(!g_server) return;
  String current = WiFi.hostname();
  String html; html.reserve(2048);
  html += F("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>");
  html += "<title>" + String(PORTAL_TITLE) + " • Hostname</title>";
  html += F("<style>*{box-sizing:border-box}html,body{margin:0;padding:0;font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;background:#222;color:#fff}.wrap{display:flex;justify-content:center;padding:24px}.container{width:100%;max-width:520px}.card{background:#111;border:1px solid #333;border-radius:8px;padding:16px;box-shadow:0 1px 0 rgba(0,0,0,.4)}h2{margin:0 0 8px 0;font-weight:600}h3{margin:0 0 16px 0;font-weight:500;color:#bbb}.button{display:inline-block;border:0;border-radius:4px;background:#1fa3ec;color:#fff;padding:10px 16px;text-decoration:none;cursor:pointer}.button:hover{background:#0f78a0}.row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}.row .button{flex:1 1 180px;text-align:center}input[type=text]{width:100%;padding:10px;border:1px solid #444;border-radius:6px;background:#181818;color:#fff}</style>");
  html += F("</head><body><div class='wrap'><div class='container'><div class='card'>");
  html += "<h2>" + String(PORTAL_TITLE) + "</h2><h3>Change Hostname</h3>";
  html += F("<form method='POST' action='/host/save'>");
  html += F("<label>Hostname</label><input type='text' name='hostname' maxlength='31' value='"); html += current; html += F("'>");
  html += F("<div class='row'><button class='button' type='submit'>Save & Reboot</button><a class='button' href='/'>Cancel</a></div>");
  html += F("<p style='color:#aaa;font-size:12px;margin-top:8px'>Letters, numbers and hyphens only. Will reboot to apply mDNS/SSDP.</p>");
  html += F("</form></div></div></div></body></html>");
  g_server->send(200,"text/html",html);
}
static void handleHostSave(){
  if(!g_server) return;
  String newh = g_server->arg("hostname");
  newh = sanitizeHostname(newh);
  if(!newh.length()) newh = defaultHostname();
  saveHostnameToEEPROM(newh);
  String html; html.reserve(1024);
  html += F("<html><head><meta http-equiv='refresh' content='5;url=/'>");
  html += F("<style>body{font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;background:#222;color:#fff;padding:24px}</style>");
  html += F("</head><body><h3>Hostname saved.</h3><p>Rebooting to apply <b>"); html += newh; html += F("</b>...</p></body></html>");
  g_server->send(200,"text/html",html);
  delay(300);
  ESP.restart();
}

// ----- Scene storage helpers -----
static inline uint8_t clampMode(int v){ return (v<0)?0: (v>2?2:v); }
static uint8_t sceneBase(uint8_t idx){ switch(idx){case 0: return EE_PROF0; case 1: return EE_PROF1; case 2: return EE_PROF2; default: return EE_PROF0; } }
static void loadScene(uint8_t idx, uint8_t out[8]){ uint8_t base=sceneBase(idx); for(uint8_t i=0;i<8;i++){ uint8_t v=EEPROM.read(base+i); if(v>2) v=0; out[i]=v; } }
static void saveScene(uint8_t idx, const uint8_t in[8]){ uint8_t base=sceneBase(idx); for(uint8_t i=0;i<8;i++){ EEPROM.write(base+i, clampMode(in[i])); } EEPROM.commit(); }
static void applyScene(uint8_t idx){ uint8_t vals[8]; loadScene(idx, vals); for(uint8_t i=0;i<8;i++){ ledMode[i]=(LedMode)vals[i]; } EEPROM.write(EE_ACTIVE, idx); EEPROM.commit(); }
static uint8_t getActiveScene(){ uint8_t a=EEPROM.read(EE_ACTIVE); if(a>2) a=0; return a; }
static void setActiveScene(uint8_t idx){ if(idx>2) idx=0; EEPROM.write(EE_ACTIVE, idx); EEPROM.commit(); }

// ----- /dynled handlers -----
static void handleDynLed(){
  if(!g_server) return;
  uint8_t s0[8], s1[8], s2[8]; loadScene(0,s0); loadScene(1,s1); loadScene(2,s2);
  uint8_t active = getActiveScene();
  String html; html.reserve(8192);
  html += F("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>");
  html += "<title>" + String(PORTAL_TITLE) + " • Dynamic LEDs</title>";
  html += F("<style>");
  html += F("*{box-sizing:border-box}html,body{margin:0;padding:0;font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;background:#222;color:#fff}");
  html += F(".wrap{display:flex;justify-content:center;padding:24px}.container{width:100%;max-width:1024px}");
  html += F(".grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}");
  html += F("@media(max-width:860px){.grid{grid-template-columns:1fr}}");
  html += F(".card{background:#111;border:1px solid #333;border-radius:8px;padding:16px;box-shadow:0 1px 0 rgba(0,0,0,.4)}");
  html += F("h2{margin:0 0 8px 0;font-weight:600}h3{margin:0 0 12px 0;font-weight:500;color:#bbb}");
  html += F(".button{display:inline-block;border:0;border-radius:4px;background:#1fa3ec;color:#fff;padding:10px 16px;text-decoration:none;cursor:pointer}.button:hover{background:#0f78a0}");
  html += F("table{width:100%;border-collapse:collapse;margin:8px 0}td{padding:6px 6px;vertical-align:middle}td:first-child{opacity:.9}");
  html += F("select{width:100%;padding:8px 10px;border:1px solid #444;border-radius:6px;background:#181818;color:#fff;appearance:none}");
  html += F(".row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}.row .button{flex:1 1 140px;text-align:center}");
  html += F(".badge{display:inline-block;margin-left:8px;padding:2px 8px;background:#2a2a2a;border:1px solid #444;border-radius:999px;font-size:12px;color:#9fe}");
  html += F(".topbar{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin:0 0 12px 0}");
  html += F(".sticky{position:sticky;top:0;z-index:10;background:#222;padding:8px 0 10px 0;border-bottom:1px solid #333}");
  html += F(".topbar .button{min-width:120px}");
  html += F("@media(max-width:480px){.topbar .button{flex:1 1 30%}}");
  html += F("</style></head><body>");
  html += F("<div class='wrap'><div class='container'>");
  html += "<h2>" + String(PORTAL_TITLE) + "</h2><h3>Dynamic LEDs</h3>";
  html += F("<div class='topbar sticky'>");
  html += F("<form method='POST' action='/dynled/apply'><input type='hidden' name='profile' value='0'><button class='button' type='submit'>Apply A<\/button><\/form>");
  html += F("<form method='POST' action='/dynled/apply'><input type='hidden' name='profile' value='1'><button class='button' type='submit'>Apply B<\/button><\/form>");
  html += F("<form method='POST' action='/dynled/apply'><input type='hidden' name='profile' value='2'><button class='button' type='submit'>Apply C<\/button><\/form>");
  html += F("<\/div>");
  html += F("<div class='grid'>");
  for(uint8_t p=0;p<3;p++){
    uint8_t* sv = (p==0)?s0: (p==1)?s1: s2;
    html += F("<div class='card'>");
    html += String("<h3>Scene ") + char('A'+p) + "</h3>";
    if(active==p) html += F("<span class='badge'>Active</span>");
    html += String("<form method='POST' action='/dynled/save?profile=") + String(p) + "'>";
    html += F("<table>");
    for(uint8_t i=0;i<8;i++){
      html += "<tr><td>LED" + String(i) + "</td><td><select name='m" + String(i) + "'>";
      html += optionFor(sv[i]);
      html += F("</select></td></tr>");
    }
    html += F("</table><div class='row'>");
    html += F("<button class='button' type='submit'>Save</button>");
    html += F("</div></form>");
    
    html += F("</div>");
  }
  html += F("</div>");
  html += F("<div class='row' style='margin-top:12px'><a class='button' href='/'>Back</a></div>");
  html += F("</div></div></body></html>");
  g_server->send(200, "text/html", html);
}

static void handleDynSave(){
  if(!g_server) return;
  int p = g_server->arg("profile").toInt(); if(p<0) p=0; if(p>2) p=2;
  uint8_t vals[8];
  for(uint8_t i=0;i<8;i++){
    String key = "m" + String(i);
    int v = g_server->hasArg(key) ? g_server->arg(key).toInt() : 0;
    vals[i] = clampMode(v);
  }
  saveScene((uint8_t)p, vals);
  g_server->sendHeader("Location","/dynled");
  g_server->send(303);
}

static void handleDynApply(){
  if(!g_server) return;
  int p = g_server->arg("profile").toInt(); if(p<0) p=0; if(p>2) p=2;
  applyScene((uint8_t)p);
  { char msg[40]; snprintf(msg, sizeof(msg), "{\"event\":\"active\",\"profile\":%d}", p); ws.broadcastTXT(msg); }
  g_server->sendHeader("Location","/dynled");
  g_server->send(303);
}

static void handleDynState(){
  uint8_t s0[8], s1[8], s2[8]; loadScene(0,s0); loadScene(1,s1); loadScene(2,s2);
  uint8_t active = getActiveScene();
  String js; js.reserve(256);
  js += "{\"active\":" + String(active) + ",\"scenes\":[";
  auto emit=[&js](uint8_t* s){ js += "["; for(uint8_t i=0;i<8;i++){ if(i) js += ","; js += String((int)s[i]); } js += "]"; };
  emit(s0); js += ","; emit(s1); js += ","; emit(s2); js += "]}";
  g_server->send(200, "application/json", js);
}

// ----- WebSocket handler (apply-only minimal) -----
void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    wsClientCount++;
    Serial.printf("[WS] connected #%u (clients=%u)\n", num, (unsigned)wsClientCount);
    return;
  }
  if (type == WStype_DISCONNECTED) {
    if(wsClientCount) wsClientCount--;
    Serial.printf("[WS] disconnected #%u (clients=%u)\n", num, (unsigned)wsClientCount);
    return;
  }
  if (type != WStype_TEXT) return;

  int p = -1;
  if (len == 1 && payload[0] >= '0' && payload[0] <= '2') {
    p = payload[0] - '0';                    // "0" / "1" / "2"
  } else if (len >= 6 && strncmp((const char*)payload, "apply:", 6) == 0) {
    p = atoi((const char*)payload + 6);      // "apply:0" etc
  }
  if (p >= 0 && p <= 2) {
    applyScene((uint8_t)p);
    char msg[40];
    snprintf(msg, sizeof(msg), "{\"event\":\"active\",\"profile\":%d}", p);
    ws.broadcastTXT(msg);
  } else {
    ws.sendTXT(num, "{\"event\":\"error\",\"msg\":\"bad profile\"}");
  }
}


// ---------------- SSDP helpers ----------------
static String macNoColons(){
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac;
}
static String stableUUID(){
  String m = macNoColons();
  while(m.length()<12) m = "0" + m;
  char buf[9]; snprintf(buf, sizeof(buf), "%08X", ESP.getChipId());
  String base = m + String(buf) + m; base = base.substring(0,32);
  return base.substring(0,8) + "-" + base.substring(8,12) + "-" + base.substring(12,16) + "-" +
         base.substring(16,20) + "-" + base.substring(20,32);
}

// ===== Field Menu / Service Tools (ENTER-to-start) =====
static const uint16_t SERIAL_MENU_WINDOW_MS = 4000; // time to press ENTER at boot

static void clearHostnameEEPROM(){
  EEPROM.write(EE_HOSTLEN, 0); // mark as unset
  EEPROM.commit();
}
static void resetLedModes(){
  for(uint8_t i=0;i<8;i++) EEPROM.write(EE_MODES+i,(uint8_t)LED_OFF);
  EEPROM.commit();
}
static void printStatus(){
  Serial.println(F("\n=== Harlech Status ==="));
  Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());
  Serial.printf("Mode: %s\n", (WiFi.getMode()==WIFI_AP) ? "AP" : "STA");
  IPAddress ip = (WiFi.getMode()==WIFI_AP)? WiFi.softAPIP() : WiFi.localIP();
  Serial.printf("IP: %s\n", ip.toString().c_str());
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println(F("======================\n"));
}

// Periodic health logging to Serial (compact one-liner)
static void logHealth(){
  bool portal = wm.getConfigPortalActive();
  const char* mode = (WiFi.getMode()==WIFI_AP) ? "AP" : "STA";
  IPAddress ip = (WiFi.getMode()==WIFI_AP)? WiFi.softAPIP() : WiFi.localIP();
  unsigned long uptime = millis()/1000UL;
  uint8_t scene = getActiveScene();
  Serial.printf("[HEALTH] mode=%s ip=%s rssi=%d heap=%u portal=%s ws=%u scene=%u up=%lus\n",
                mode,
                ip.toString().c_str(),
                WiFi.RSSI(),
                ESP.getFreeHeap(),
                portal?"on":"off",
                (unsigned)wsClientCount,
                (unsigned)scene,
                uptime);
}


static void fieldMenu(){
  Serial.println(F("\n=== Harlech Field Menu ==="));

  Serial.println(F("[F] Factory reset (WiFi + hostname + LED modes)"));
  Serial.println(F("[W] Reset WiFi credentials only"));
  Serial.println(F("[H] Clear hostname (reverts to default)"));
  Serial.println(F("[L] Reset LED modes to Off"));
  Serial.println(F("[P] Print status"));
  Serial.println(F("[C] Start Config Portal (AP) now"));
  Serial.println(F("[R] Reboot"));
  Serial.println();

  for(;;){
    while(!Serial.available()) { yield(); }
    char c = toupper(Serial.read());
    switch(c){
      case 'F':
        Serial.println(F("Factory reset..."));
        wm.resetSettings();
        clearHostnameEEPROM();
        resetLedModes();
        delay(200); ESP.restart();
        break;
      case 'W':
        Serial.println(F("Reset WiFi credentials..."));
        wm.resetSettings();
        delay(200); ESP.restart();
        break;
      case 'H':
        Serial.println(F("Clearing hostname..."));
        clearHostnameEEPROM();
        delay(200); ESP.restart();
        break;
      case 'L':
        Serial.println(F("Resetting LED modes to Off..."));
        resetLedModes();
        delay(200); ESP.restart();
        break;
      case 'P':
        printStatus();
        break;
      case 'C': {
        Serial.println(F("Starting Config Portal (AP). Close it to return here."));
        String ap = WiFi.hostname(); if(!ap.length()) ap = "Harlech";
        wm.setConfigPortalBlocking(true);
        wm.startConfigPortal(ap.c_str()); // blocks until portal exits
        Serial.println(F("Portal closed.")); printStatus();
      } break;
      case 'R':
        Serial.println(F("Rebooting..."));
        delay(200); ESP.restart();
        break;
    }
  }
}

static void maybeEnterFieldMenu(){
  Serial.printf("\nPress ENTER within %u ms for Field Menu...\n", SERIAL_MENU_WINDOW_MS);
  uint32_t t0 = millis();
  while(millis() - t0 < SERIAL_MENU_WINDOW_MS){
    if(Serial.available()){
      int ch = Serial.read();
      if(ch == '\r' || ch == '\n'){ fieldMenu(); }
    }
    delay(10);
  }
}

// ---------------- Setup / Loop ----------------
void setup(){
  Serial.begin(115200);
  delay(50);

  // Give a short window to press ENTER and enter the field menu
  maybeEnterFieldMenu();

  pinMode(D6, OUTPUT); pinMode(D5, OUTPUT); pinMode(D8, OUTPUT); pinMode(D7, OUTPUT);
  pinLow(GPIO_OE);  // enable 74HC595 outputs (OE is active-LOW)

  if(CONFIG_LED_PIN >= 0){
    pinMode(CONFIG_LED_PIN, OUTPUT);
    digitalWrite(CONFIG_LED_PIN, CONFIG_LED_ACTIVE_LOW ? HIGH : LOW); // off
  }

  EEPROM.begin(EEPROM_SIZE);
  if(EEPROM.read(EE_SIG)==EEPROM_SIG){
    for(uint8_t i=0;i<8;i++){ uint8_t m = EEPROM.read(EE_MODES+i); ledMode[i] = (m<=LED_CANDLE)?(LedMode)m:LED_OFF; }
  }else{
    for(uint8_t i=0;i<8;i++) EEPROM.write(EE_MODES+i,(uint8_t)LED_OFF);
    // initialize scenes A/B/C to all Off and active=0
    for(uint8_t i=0;i<8;i++){ EEPROM.write(EE_PROF0+i,(uint8_t)LED_OFF); EEPROM.write(EE_PROF1+i,(uint8_t)LED_OFF); EEPROM.write(EE_PROF2+i,(uint8_t)LED_OFF); }
    EEPROM.write(EE_ACTIVE, 0);
    EEPROM.write(EE_SIG,EEPROM_SIG); EEPROM.commit();
  }

  initFlicker();
  randomSeed(ESP.getCycleCount());

  // Hostname (load from EEPROM or default)
  String host = loadHostnameFromEEPROM();
  if(!host.length()) host = defaultHostname();
  WiFi.hostname(host);
  wm.setHostname(host);
  wm.setConfigPortalBlocking(false);
  // Customize WiFiManager portal title (affects AP + STA portals)
  wm.setTitle(PORTAL_TITLE);
  if(WM_DARK_MODE){
    wm.setCustomHeadElement(R"(<style>
:root{color-scheme:dark;}
html,body{background:#121212;color:#eaeaea;}
a{color:#1fa3ec}
input,select,textarea{background:#1e1e1e;color:#eaeaea;border:1px solid #333;}
.button,button,input[type=button],input[type=submit]{background:#1fa3ec;color:#fff;}
.button:hover,button:hover,input[type=button]:hover,input[type=submit]:hover{background:#0f78a0}
hr{border-color:#333}
</style>)");
  }

  // Ensure WiFiManager home (AP & STA) shows our custom buttons
  {
    std::vector<const char*> menu = {"wifi","info","param","custom","sep","restart","exit"};
    wm.setMenu(menu);
    wm.setCustomMenuHTML("<form action='/led' method='get'><button class='button' style='min-width:200px'>Configure LEDs</button></form>\
<form action='/dynled' method='get' style='margin-top:8px'><button class='button' style='min-width:200px'>Dynamic Scenes</button></form>\
<form action='/host' method='get' style='margin-top:8px'><button class='button' style='min-width:200px'>Change Hostname</button></form>");
  }

  // Prepare persistent UUID for SSDP/description.xml
  gUUID = stableUUID();

  // Connect (or open AP portal). After connecting, start WM portal in STA mode at ROOT
  bool ok = wm.autoConnect(host.c_str());
  if(!ok){
    Serial.println(F("[WiFi] autoConnect failed; AP portal likely active."));

    // Attach our routes to the AP portal as well
    g_server = wm.server.get();
    if(g_server){
      g_server->on("/health", HTTP_GET, [](){ g_server->send(200, "application/json", "{\"ok\":true}"); });
      g_server->on("/led", HTTP_GET, handleRoot);
      g_server->on("/led/save", HTTP_POST, handleSave);
      g_server->on("/host", HTTP_GET, handleHost);
      g_server->on("/host/save", HTTP_POST, handleHostSave);
      g_server->on("/dynled", HTTP_GET, handleDynLed);
      g_server->on("/dynled/save", HTTP_POST, handleDynSave);
      g_server->on("/dynled/apply", HTTP_POST, handleDynApply);
      g_server->on("/dynled/state", HTTP_GET, handleDynState);
      g_server->on("/description.xml", HTTP_GET, [](){
        IPAddress ip = (WiFi.getMode()==WIFI_AP)? WiFi.softAPIP() : WiFi.localIP();
        String ipS = ip.toString();
        String friendly = WiFi.hostname().length() ? WiFi.hostname() : String("Harlech Controller");
        String xml; xml.reserve(1024);
        xml  = F("<?xml version=\"1.0\"?>");
        xml += F("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">");
        xml += F("<specVersion><major>1</major><minor>0</minor></specVersion>");
        xml += F("<device>");
        xml += F("<deviceType>upnp:rootdevice</deviceType>");
        xml += F("<friendlyName>"); xml += friendly; xml += F("</friendlyName>");
        xml += F("<manufacturer>TerrainTronics</manufacturer>");
        xml += F("<manufacturerURL>http://terraintronics.com</manufacturerURL>");
        xml += F("<modelName>"); xml += PORTAL_TITLE; xml += F("</modelName>");
        xml += F("<modelNumber>1.0</modelNumber>");
        xml += F("<serialNumber>"); xml += String(ESP.getChipId(), HEX); xml += F("</serialNumber>");
        xml += F("<presentationURL>http://"); xml += ipS; xml += F("/</presentationURL>");
        xml += F("<UDN>uuid:"); xml += gUUID; xml += F("</UDN>");
        xml += F("</device></root>");
        g_server->send(200, "text/xml", xml);
      });
      // Start WS in AP mode too (for local tools)
      ws.begin(); ws.onEvent(onWsEvent);
    }
  } else {
    Serial.print(F("[WiFi] connected, IP: ")); Serial.println(WiFi.localIP());
    Serial.print(F("[HTTP] Browse STA portal at: http://")); Serial.print(WiFi.localIP()); Serial.println(F("/"));    // (menu injected earlier for both AP & STA)

    // Start WiFiManager's Web Portal in STA mode; it serves at ROOT (/) 
    wm.startWebPortal();
    g_server = wm.server.get();
    if(g_server){      // Lightweight health probe for quick sanity checks
      g_server->on("/health", HTTP_GET, [](){ g_server->send(200, "application/json", "{\"ok\":true}"); });
      // IMPORTANT: Do NOT override "/" — let WiFiManager serve its home page (with our custom buttons) in both STA and AP modes.

      g_server->on("/led", HTTP_GET, handleRoot);
      g_server->on("/led/save", HTTP_POST, handleSave);
      g_server->on("/description.xml", HTTP_GET, [](){
        IPAddress ip = (WiFi.getMode()==WIFI_AP)? WiFi.softAPIP() : WiFi.localIP();
        String ipS = ip.toString();
        String friendly = WiFi.hostname().length() ? WiFi.hostname() : String("Harlech Controller");
        String xml;
        xml.reserve(1024);
        xml  = F("<?xml version=\"1.0\"?>");
        xml += F("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">");
        xml += F("<specVersion><major>1</major><minor>0</minor></specVersion>");
        xml += F("<device>");
        xml += F("<deviceType>upnp:rootdevice</deviceType>");
        xml += F("<friendlyName>"); xml += friendly; xml += F("</friendlyName>");
        xml += F("<manufacturer>TerrainTronics</manufacturer>");
        xml += F("<manufacturerURL>http://terraintronics.com</manufacturerURL>");
        xml += F("<modelName>"); xml += PORTAL_TITLE; xml += F("</modelName>");
        xml += F("<modelNumber>1.0</modelNumber>");
        xml += F("<serialNumber>"); xml += String(ESP.getChipId(), HEX); xml += F("</serialNumber>");
        xml += F("<presentationURL>http://"); xml += ipS; xml += F("/</presentationURL>");
        xml += F("<UDN>uuid:"); xml += gUUID; xml += F("</UDN>");
        xml += F("</device></root>");
        g_server->send(200, "text/xml", xml);
      });

      // Hostname UI
      g_server->on("/host", HTTP_GET, handleHost);
      g_server->on("/host/save", HTTP_POST, handleHostSave);
      // Dynamic LED scenes
      g_server->on("/dynled", HTTP_GET, handleDynLed);
      g_server->on("/dynled/save", HTTP_POST, handleDynSave);
      g_server->on("/dynled/apply", HTTP_POST, handleDynApply);
      g_server->on("/dynled/state", HTTP_GET, handleDynState);
      // WebSocket for scene switching
      ws.begin();      ws.onEvent(onWsEvent);
    }
    else {
      // Rare: if WiFiManager did not spin up a server in STA mode, start our own as a fallback
      static ESP8266WebServer fallback(80);
      g_server = &fallback;
            // Fallback home (only if WiFiManager didn't start a server). Provide a simple landing with the same custom buttons.
      g_server->on("/", HTTP_GET, [](){
        String html; html.reserve(2048);
        html += F("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>");
        html += "<title>" + String(PORTAL_TITLE) + " • Home</title>";
        html += F("<style>*{box-sizing:border-box}html,body{margin:0;padding:0;font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;background:#222;color:#fff}.wrap{display:flex;justify-content:center;padding:24px}.container{width:100%;max-width:520px}.card{background:#111;border:1px solid #333;border-radius:8px;padding:16px;box-shadow:0 1px 0 rgba(0,0,0,.4)}h2{margin:0 0 8px 0;font-weight:600}h3{margin:0 0 16px 0;font-weight:500;color:#bbb}.button{display:inline-block;border:0;border-radius:4px;background:#1fa3ec;color:#fff;padding:10px 16px;text-decoration:none;cursor:pointer}.button:hover{background:#0f78a0}.row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}.row .button{flex:1 1 180px;text-align:center}</style>");
        html += F("</head><body><div class='wrap'><div class='container'><div class='card'>");
        html += "<h2>" + String(PORTAL_TITLE) + "</h2><h3>Home</h3>";
        html += F("<div class='row'>");
        html += F("<form action='/led' method='get'><button class='button' type='submit'>Configure LEDs</button></form>");
        html += F("<form action='/dynled' method='get'><button class='button' type='submit'>Dynamic Scenes</button></form>");
        html += F("<form action='/host' method='get'><button class='button' type='submit'>Change Hostname</button></form>");
        html += F("</div><p style='color:#aaa;font-size:12px;margin-top:8px'>WiFiManager portal fallback page.</p>");
        html += F("</div></div></div></body></html>");
        g_server->send(200, "text/html", html);
      });
      g_server->on("/health", HTTP_GET, [](){ g_server->send(200, "application/json", "{\"ok\":true}"); });
      g_server->on("/led", HTTP_GET, handleRoot);
      g_server->on("/led/save", HTTP_POST, handleSave);
      g_server->on("/host", HTTP_GET, handleHost);
      g_server->on("/host/save", HTTP_POST, handleHostSave);
      g_server->on("/dynled", HTTP_GET, handleDynLed);
      g_server->on("/dynled/save", HTTP_POST, handleDynSave);
      g_server->on("/dynled/apply", HTTP_POST, handleDynApply);
      g_server->on("/dynled/state", HTTP_GET, handleDynState);
      g_server->begin();
      Serial.println(F("[HTTP] Fallback web server started on port 80"));
    }

    // SSDP announce
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName(host.c_str());
    SSDP.setManufacturer("TerrainTronics");
    SSDP.setManufacturerURL("http://terraintronics.com");
    SSDP.setModelName(PORTAL_TITLE);
    SSDP.setModelNumber("1.0");
    {
      String abs = String("http://") + WiFi.localIP().toString() + "/"; // absolute URL preferred by Windows
      SSDP.setModelURL(abs.c_str());
    }
    SSDP.setURL("/");
    SSDP.setDeviceType("upnp:rootdevice"); // advertise as root device (Explorer-friendly)
    SSDP.setSerialNumber(String(ESP.getChipId(), HEX));
    SSDP.setUUID(gUUID.c_str()); // ensure USN matches <UDN> in description.xml
    if (!SSDP.begin()) {
      Serial.println(F("SSDP failed to start"));
    } else {
      Serial.println(F("SSDP started"));
    }

    // mDNS (optional)
    if(MDNS.begin(host.c_str())){
      MDNS.addService("http", "tcp", 80);
      Serial.println(F("[mDNS] started"));
    }
  }
  // One-shot health log on boot
  logHealth();
  lastHealthLog = millis();  // schedule next tick ~HEALTH_PERIOD_MS later
}


void loop(){
  wm.process();         // keep WM portal responsive (AP or STA)

  // Blink config LED while the config portal is active
  if(CONFIG_LED_PIN >= 0){
    static uint32_t lastBlink = 0;
    static bool blink = false;
    bool active = wm.getConfigPortalActive();
    uint32_t now = millis();
    if(active){
      if(now - lastBlink >= CONFIG_LED_PERIOD){
        lastBlink = now; blink = !blink;
        digitalWrite(CONFIG_LED_PIN, (blink ^ !CONFIG_LED_ACTIVE_LOW) ? HIGH : LOW);
      }
    }else{
      digitalWrite(CONFIG_LED_PIN, CONFIG_LED_ACTIVE_LOW ? HIGH : LOW);
    }
  }

  updateFlicker();
  updatePWM();
  ws.loop();

  // Periodic health log to Serial
  if (millis() - lastHealthLog >= HEALTH_PERIOD_MS) {
    lastHealthLog = millis();
    logHealth();
  }
}

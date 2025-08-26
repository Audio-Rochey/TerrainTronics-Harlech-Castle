# Harlech WiFiMgr + WebSockets

A tiny Wi‑Fi–configurable web app and WebSocket controller for TerrainTronics’ **Harlech Castle** LED boards (ESP8266 Wemos D1 Mini). Create lighting scenes in the browser, then switch scenes instantly from the site **or** any WebSocket client (e.g., Stream Deck via a helper script).

> Designed for quick demos, tabletop lighting, and maker projects. No external cloud. Everything is served from the ESP8266.

---

## ✨ Features

- **Zero‑app setup** with Wi‑Fi Manager captive portal (Connect to it's own WiFi on first boot, and provide your own wifi credentials).
- **Web UI** with a **Dynamic Scenes** page to build and preview Scene A/B/C.
- **Fast scene switching** via WebSockets (works from browser **and** external controllers like an Elgato streamdeck).
- **mDNS/SSDP discovery** (optional) for easy finding on the LAN.
- **OTA‑friendly** structure and static assets in LittleFS/ SPIFFS.

> This repo reflects our internal “Harlech WifiMgr and WebSockets” milestones. It pairs well with the TerrainTronics **Harlech Castle** board and a **Wemos D1 Mini**.

---

## 🧰 Hardware

- **MCU:** ESP8266 Wemos D1 Mini (4 MB flash recommended)
- **LED target:** TerrainTronics **Harlech Castle** (5× LED outputs / shift‑register driven) or similar
- **Power:** 5 V (ensure adequate current for LEDs)

---

## 🚀 Easy to set up on a Wemos d1 mini

1. **Get your Wemos D1 Mini from Amazon**.
2. Connect USB to your PC and open your browser to ([https://audio-rochey.github.io/ESP-Web-Tools/](https://audio-rochey.github.io/ESP-Web-Tools/))
3. Look for Harlech WiFi Manager & Web Sockets
4. Press Upload!



## 🚀 Quick Start for modifying and building code

1. **Clone** the repo into Arduino IDE 2.x or PlatformIO.
2. Install the **ESP8266 core** for Arduino.
3. Install libraries listed at the top of the sketch (WiFiManager, WebSockets, SSDP/mDNS if enabled).
4. **Build & Upload** the firmware.
5. On first boot, connect to the captive portal Wi‑Fi and enter your network credentials.
6. Browse to the device (e.g., `http://harlech.local/` via mDNS) and click **Dynamic Scenes**.

> **Tip:** If your board uses file system assets, upload the LittleFS/SPIFFS data folder after flashing.

---

## 🖥️ Web UI Overview

\*\*Home (Wi‑Fi Manager portal) \*\*\
On first boot or whenever Wi‑Fi isn’t configured, the controller starts its own access‑point and serves the **Wi‑Fi Manager** home page. The Wifi SSID to connect to will be something like Harlech-XYZ123 (the last digits derived from an internal board ID)



You can find the configuration page at [http://192.168.4.1](http://192.168.4.1)



When already connected to your wifi, the same home page is available at your device’s IP/hostname and shows status ***plus*** large buttons to:

- **Configure LEDs** (`/led`)
- **Dynamic Scenes** (`/dynled`)
- **Change Hostname** (`/host`)

**Wi‑Fi Configuration**\
Use **Configure WiFi** on the home page to set SSID and password. After saving, the unit connects as a station and continues to serve the UI at `http://<ip>/`. To reconfigure later, bring up the Wi‑Fi portal again (e.g., power up where your usual SSID is unavailable or clear credentials via serial/flash workflows). If your OS doesn’t resolve `.local` hostnames, browse by IP. You can find the device IP address by watching the Serial port data as the device boots up. (

\*\*Fixed Lighting (per‑channel) \*\*\
Set each of the **8 outputs** to **Off / Solid / Candle** and click **Save**. This applies immediately and is persisted in EEPROM as your default “fixed look.” Use this when you want a simple, always‑on profile without scene switching.

\*\*Dynamic Lighting Scenes \*\*\
Build three profiles **Scene A / Scene B / Scene C**, each with 8 per‑channel modes (Off/Solid/Candle). A sticky **Apply A/B/C** bar at the top lets you push a profile live instantly. Scenes are stored in EEPROM and can be switched from the web page or over WebSocket. Programmatic state is available at `` (JSON). Applying a scene also emits a broadcast: `{"event":"active","profile":N}`.

**Hostname — \*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\***``\
Choose a DNS‑friendly name (letters, numbers, hyphen). Saving triggers a short reboot and updates **mDNS** and **SSDP** announcements.

**Why change the hostname?**

- Run **multiple controllers** on one LAN without collisions.
- Use memorable endpoints: `http://harlech‑stage.local/`, `ws://harlech‑stage.local:82/`.
- Avoid DHCP/OS auto‑renames and keep bookmarks stable.
- Make Stream Deck / Companion actions easier to read and maintain.

---

## 🎥 Demo videos

- Demo 1 — Harlech WiFiMgr + Dynamic Scenes: [https://youtu.be/c0mubq7XFXU](https://youtu.be/c0mubq7XFXU)
- Demo 2 — Stream Deck / WebSocket control: [https://youtu.be/dTnSNvuOWLM](https://youtu.be/dTnSNvuOWLM)

## 🔌 WebSocket API (as implemented in this firmware)

**Endpoint:** `ws://<device-ip>:82/`  (or `ws://<hostname>.local:82/`)

**Protocol:** apply‑only commands. The socket accepts a one‑character profile index or the explicit `apply:` form.

### Commands (client → device)

```
"0"              # Apply Scene A
"1"              # Apply Scene B
"2"              # Apply Scene C
apply:0          # Also valid (apply:1, apply:2)
```

### Broadcasts (device → all clients)

```
{"event":"active","profile":0}   # after a scene is applied
{"event":"error","msg":"bad profile"}   # on invalid input
```

### Quick test in a browser console

```js
const W = new WebSocket('ws://<IP>:82/');
W.onmessage = (e) => console.log('MSG:', e.data);
W.onopen    = () => W.send('0');   // apply Scene A
```

> Notes • Scenes are created/edited on **/dynled**; WebSocket is **apply only**. • Applying from the website also triggers the same `{event:"active"}` broadcast. • Ensure port **82** is reachable on your LAN (some firewalls block non‑80 ports).

### HTTP endpoints (optional)

- `POST /dynled/apply` with form field `profile=0|1|2` → applies a scene and returns **303 Redirect** to `/dynled`.
- `GET  /dynled/state` → JSON snapshot like `{ "active":1, "scenes":[ [..8..],[..8..],[..8..] ] }`.

## 🎛️ Using with Elgato Stream Deck

**Recommended:** Use the free **Web Requests** plugin for Stream Deck (by Adrian Mullings) — it can send HTTP **or WebSocket** requests directly from a button. Install from the Elgato Marketplace: [Web Requests plugin](https://marketplace.elgato.com/product/web-requests-d7d46868-f9c8-4fa5-b775-ab3b9a7c8add).

### Quick setup (WebSocket)

1. Add a **Web Requests** action and set **Mode = WebSocket**.
2. **URL:** `ws://<device-ip>:82/`
3. **Message (text):** `0`  *(Scene A)*\
   — or use `1` *(Scene B)*, `2` *(Scene C)*, or the explicit `apply:0|1|2`.
4. (Optional) Enable auto‑close after send.

Duplicate buttons for each scene.

**HTTP (alternative):** set **Mode = HTTP**, **Method = POST**, **URL:** `http://<device-ip>/dynled/apply`, **Body:** `profile=0`, **Content‑Type:** `application/x-www-form-urlencoded`. (This performs the same action and the device will broadcast the `active` event.)

### Alternative / fallback

If you prefer not to install a plugin, you can still use tiny helpers:

- **Node.js one‑liner**

```js
const WebSocket = require('ws');
const ws = new WebSocket('ws://DEVICE_IP:82/');
ws.on('open', ()=>{ ws.send('0'); setTimeout(()=>ws.close(), 200); });
```

- **Browser bookmarklet** (on a nearby PC)

```text
javascript:(()=>{const ws=new WebSocket('ws://DEVICE_IP:82/');ws.onopen=()=>ws.send('0');setTimeout(()=>ws.close(),300);})();
```

> The plugin also supports **HTTP**; if you expose simple REST endpoints on the device, you can trigger those too.

---

## ⚙️ Build Notes

- **WiFi Manager:** long‑press or erase config to re‑enter captive portal.
- **Discovery:** enable **mDNS** (`harlech.local`) and/or **SSDP** if your network supports it.
- **File system:** store static assets (HTML/CSS/JS) in `data/` and upload to LittleFS/SPIFFS.
- **OTA:** the sketch is organized to keep assets small; you can add Arduino OTA if desired.

---

## 🧪 Troubleshooting

- Can’t find the device? Try `http://<ip>/` directly, or use `harlech.local` if mDNS works on your OS.
- WebSocket not connecting? Check that your PC/phone is on the **same LAN** and no captive portal is active.
- Scenes not changing from Stream Deck? Test the WebSocket directly: connect to `ws://<ip>:82/` and send `"1"` (Scene B). See the API section above.

---

## 🗃️ Backup & Restore (esptool)

Read full flash (example for a 4 MB ESP8266 at high baud):

```bash
esptool.py --chip esp8266 --port COM5 --baud 921600 read_flash 0 0x400000 backup.bin
```

Write it back later:

```bash
esptool.py --chip esp8266 --port COM5 --baud 460800 write_flash 0 backup.bin
```

> Replace `COM5` with your serial port and adjust the size (0x400000 = 4 MB). Use `esptool.py flash_id` to confirm flash size.

---

## 🛒 Where to Buy / Follow

- TerrainTronics on **Etsy** and **Tindie** for Harlech‑series boards and kits.
- Recommended MCU: “**Wemos D1 Mini**” (ESP8266, 4 MB flash). Many vendors carry genuine or compatible boards.
- **YouTube:** TerrainTronics channel for demos, build logs, and setup videos.

*(Links will be added once this README lands in the public repo.)*

---

## 🤝 Contributing

Issues and PRs are welcome! Please include your hardware details, log snippets, and steps to reproduce.

---

## 📜 License

MIT (unless otherwise noted in individual source files).

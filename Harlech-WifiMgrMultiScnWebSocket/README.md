# Harlech WiFiMgr + WebSockets

A tiny Wi‑Fi–configurable web app and WebSocket controller for TerrainTronics’ **Harlech Castle** LED boards (ESP8266 Wemos D1 Mini). Create lighting scenes in the browser, then switch scenes instantly from the site **or** any WebSocket client (e.g., Stream Deck via a helper plugin).

> Designed for quick demos, tabletop lighting, and maker projects. No external cloud. Everything is served from the ESP8266.

---

## 🎥 Demo videos

- Demo 1 — Harlech WiFiMgr + Dynamic Scenes: https://youtu.be/c0mubq7XFXU
- Demo 2 — Stream Deck / WebSocket control: https://youtu.be/dTnSNvuOWLM

---

## ✨ Features

- **Zero‑app setup** with Wi‑Fi Manager captive portal (connect to its own Wi‑Fi on first boot, then enter your Wi‑Fi credentials).
- **Web UI** with a **Dynamic Scenes** page to build and preview Scene A/B/C.
- **Fast scene switching** via WebSockets (works from the browser **and** external controllers like an Elgato Stream Deck).
- **mDNS/SSDP discovery** (optional) for easy finding on the LAN.
- **OTA‑friendly** structure and static assets in LittleFS/SPIFFS.

> This repo reflects our internal “Harlech WiFiMgr and WebSockets” milestones. It pairs well with the TerrainTronics **Harlech Castle** board and a **Wemos D1 Mini**.

---

## 🧰 Hardware

- **MCU:** ESP8266 Wemos D1 Mini (4 MB flash recommended)
- **LED target:** TerrainTronics **Harlech Castle** (5× LED outputs / shift‑register driven) or similar
- **Power:** 5 V (ensure adequate current for LEDs)

---

## 🚀 Easy setup on a Wemos D1 Mini

1. Get your Wemos D1 Mini.
2. Connect USB to your PC and open: **https://audio-rochey.github.io/ESP-Web-Tools/**
3. Find **Harlech WiFi Manager & WebSockets**.
4. Click **Upload**.

---

## 🚀 Quick Start for modifying and building code

1. **Clone** the repo into Arduino IDE 2.x or PlatformIO.
2. Install the **ESP8266 core** for Arduino.
3. Install libraries listed at the top of the sketch (WiFiManager, WebSockets, SSDP/mDNS if enabled).
4. **Build & Upload** the firmware.
5. If credentials are blank, the device opens the captive portal; enter your Wi‑Fi details.
6. Browse to the device (e.g., `http://harlech.local/`) and click **Dynamic Scenes**.

> **Tip:** If your board uses file system assets, upload the LittleFS/SPIFFS `data/` folder after flashing.

---

## 🖥️ Web UI Overview

**Home (Wi‑Fi Manager portal)**  
On first boot (or after clearing credentials), the controller starts an access point like `Harlech-XYZ123`. Connect and visit **http://192.168.4.1** to open the portal. When already on your LAN, open `http://<ip>/` or `http://<hostname>.local/`.

From the home page you can jump to:
- **Configure LEDs** (`/led`) — per‑channel fixed lighting (Off / Solid / Candle). Click **Save** to persist as the default fixed look.
- **Dynamic Scenes** (`/dynled`) — build **Scene A / Scene B / Scene C**, each defining all channels. Use the sticky **Apply A/B/C** bar to push a scene live. Scenes are stored in EEPROM and can be switched from the web page or over WebSocket. Programmatic state is available at **`GET /dynled/state`** (JSON). Applying a scene also emits a broadcast: `{"event":"active","profile":N}`.
- **Change Hostname** (`/host`) — set a DNS‑friendly name (letters, numbers, hyphen). Saving reboots and updates **mDNS** and **SSDP** announcements.

**Wi‑Fi configuration**  
Use **Configure WiFi** in the portal to set SSID/password. To reconfigure later, either clear credentials or use the **Field Menu**: open a serial terminal at boot and press **ENTER within ~4 seconds** to start the Config Portal. If your OS doesn’t resolve `.local` hostnames, browse by IP (the IP is printed to the serial console at boot).

**Why change the hostname?**
- Run **multiple controllers** on one LAN without collisions.
- Use memorable endpoints: `http://harlech‑stage.local/`, `ws://harlech‑stage.local:82/`.
- Avoid DHCP/OS auto‑renames and keep bookmarks stable.
- Make Stream Deck / Companion actions easier to read and maintain.

---

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
{"event":"active","profile":0}          # after a scene is applied
{"event":"error","msg":"bad profile"}   # on invalid input
```

### Quick test in a browser console
```js
const W = new WebSocket('ws://<IP>:82/');
W.onmessage = (e) => console.log('MSG:', e.data);
W.onopen    = () => W.send('0');   // apply Scene A
```

> Notes: Scenes are created/edited on **/dynled**; WebSocket is **apply only**. Applying from the website also triggers the same `{event:"active"}` broadcast. Ensure port **82** is reachable on your LAN (some firewalls block non‑80 ports).

### HTTP endpoints (optional)
- `POST /dynled/apply` with form field `profile=0|1|2` → applies a scene and returns **303 Redirect** to `/dynled`.
- `GET  /dynled/state` → JSON snapshot like `{ "active":1, "scenes":[ [..8..],[..8..],[..8..] ] }`.

---

## 🎛️ Using with Elgato Stream Deck

**Recommended:** Use the free **Web Requests** plugin for Stream Deck (by Adrian Mullings) — it can send HTTP **or WebSocket** requests directly from a button. Install from the Elgato Marketplace: https://marketplace.elgato.com/product/web-requests-d7d46868-f9c8-4fa5-b775-ab3b9a7c8add

### Quick setup (WebSocket)
1. Add a **Web Requests** action and set **Mode = WebSocket**.
2. **URL:** `ws://<device-ip>:82/`
3. **Message (text):** `0` *(Scene A)* — or `1` *(Scene B)*, `2` *(Scene C)*, or `apply:0|1|2`.
4. (Optional) Enable auto‑close after send.

Duplicate buttons for each scene.

**HTTP (alternative):** set **Mode = HTTP**, **Method = POST**, **URL:** `http://<device-ip>/dynled/apply`, **Body:** `profile=0`, **Content‑Type:** `application/x-www-form-urlencoded`. (This performs the same action and the device will broadcast the `active` event.)

**Fallback options**
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

---

## ⚙️ Build Notes

- **Wi‑Fi Manager:** use the **Field Menu** (press ENTER within ~4 s at boot) or erase credentials to start the captive portal.
- **Discovery:** enable **mDNS** (`harlech.local`) and/or **SSDP** if your network supports it.
- **File system:** store static assets (HTML/CSS/JS) in `data/` and upload to LittleFS/SPIFFS.
- **OTA:** the sketch is organized to keep assets small; you can add Arduino OTA if desired.

---

## 🧪 Troubleshooting

- Can’t find the device? Try `http://<ip>/` directly, or use `harlech.local` if mDNS works on your OS.
- WebSocket not connecting? Check that your PC/phone is on the **same LAN** and no captive portal is active.
- Scenes not changing from Stream Deck? Test the WebSocket directly: connect to `ws://<ip>:82/` and send `"1"` (Scene B).

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
- Recommended MCU: **Wemos D1 Mini** (ESP8266, 4 MB flash). Many vendors carry genuine or compatible boards.
- **YouTube:** TerrainTronics channel for demos, build logs, and setup videos.

*(Links will be added once this README lands in the public repo.)*

---

## 🤝 Contributing

Issues and PRs are welcome! Please include your hardware details, log snippets, and steps to reproduce.

---

## 📜 License

MIT (unless otherwise noted in individual source files).

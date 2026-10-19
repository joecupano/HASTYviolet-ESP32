# HASTYviolet-ESP32 MVP

## Overview

HASTYviolet-ESP32 is firmware that turns an ESP32 LoRa development board into a **point-to-point LoRa transceiver with its own web interface**. There is no phone app and no cloud service. The board creates (or joins) a Wi-Fi network, serves a web page from its own flash, and relays messages between your browser and the LoRa radio.

```
 Browser  <--- Wi-Fi: HTTP :8000 + WebSocket :8888 --->  ESP32  <--- SPI --->  LoRa radio  ~~~ RF ~~~  other nodes
```

How it works:

- **Web server (port 8000)** serves the UI (`data/` is stored in the SPIFFS filesystem).
- **WebSocket server (port 8888)** carries messages and radio commands between the browser and the board.
- **Radio task** runs alone on core 0 and is the only code that touches the radio. The WebSocket side runs on core 1. They exchange data through two queues (`txQueue`, `rxQueue`), which removed the cross-core radio corruption seen in earlier versions.
- **Radio layer** (`src/HASviolet_radio.h`) hides the chip difference: SX1276 via `sandeepmistry/LoRa` on the V2, SX1262 via RadioLib on the V3. `main.cpp` is identical for both.
- **Configuration** lives in two places: `src/HASviolet_config.h` (Wi-Fi, ports, default radio settings, compile time) and `data/hasVIOLET.json` (radio, contact, macros, and the ten channel presets, loaded at boot).
- **Wi-Fi**: if `HASV_WIFI_SSID`/`HASV_WIFI_KEY` are set, the board tries to join that network for about 10 seconds, then falls back to its own access point (`HASviolet` / `purple99`, IP `192.168.4.1`). Nothing is set by default, so it starts in AP mode.

What the MVP gives you:

- Text messages sent and received over LoRa from any browser, with a callsign-style header (`MYCALL-SS>DESTCALL-SS|message`).
- A radio-style front panel: ten channel presets (frequency + modem), TX power selection, four message macros.
- Pinned library versions, so the build stays reproducible.
- Optional OLED output on the board (logo at boot, last TX/RX line).

What it deliberately does not do (yet):

- No routing, relaying, acknowledgements or retries. Every packet is a plain broadcast on the current channel.
- No encryption. Messages go out as plain text, which also keeps the project usable for amateur radio, where encryption is generally not allowed.
- Beacon, radio on/off, and MODE buttons in the UI are placeholders: the firmware acknowledges them but does not act on them.
- Settings changed with dot-commands in the UI (call sign, macros) are held in the browser only and are lost on reload.

## Comparison with Meshcore and Meshtastic

Meshtastic and Meshcore are mature open-source LoRa *mesh* projects. HASTYviolet-ESP32 is a much simpler, narrower tool. The table reflects general knowledge of the three projects; check their current documentation, as both mesh projects change quickly.

| | HASTYviolet-ESP32 | Meshtastic | Meshcore |
|---|---|---|---|
| **Core idea** | Direct LoRa transceiver with a built-in web UI | Self-forming mesh of general-purpose nodes | Mesh with dedicated node roles (clients, repeaters, room servers) |
| **Topology / routing** | None. Single-hop broadcast | Managed flooding: nodes rebroadcast packets so messages hop across the mesh | Messages are routed along learned paths through repeaters, with flooding used for discovery |
| **Range extension** | Only by getting radios closer or higher | Every node can relay | Mainly through dedicated repeaters |
| **Delivery guarantees** | None | Optional acknowledgements and retries for direct messages | Acknowledged delivery along the path |
| **Encryption** | None (plain text) | Channel encryption (AES) with shared keys, plus public-key direct messages | Encrypted messages |
| **User interface** | Web page served by the board over Wi-Fi; no app | Phone apps (BLE, Wi-Fi, serial), web client, CLI | Phone/web companion apps; separate firmware per role |
| **Hardware support** | Heltec V2 and V3 only | Very wide range of ESP32, nRF52 and RP2040 boards | Wide range of boards, including ESP32 and nRF52 |
| **Radio parameters** | Fully visible and user-tunable: frequency, bandwidth, SF, CR, power per channel | Mostly presets (e.g. LongFast), tunable in settings | Presets and settings, with regional defaults |
| **Intended audience** | Experimenters, amateur radio operators, people learning LoRa/ESP32 | General off-grid and community messaging | Community and larger-scale resilient networks |
| **Complexity of firmware** | A few hundred lines you can read in an evening | Large, feature-rich codebase | Large, feature-rich codebase |

**When to choose which**

- Choose **Meshtastic** or **Meshcore** when you want a working off-grid network: multi-hop coverage, encryption, phone apps, position sharing, many supported devices.
- Choose **HASTYviolet-ESP32** when you want to *see and control the radio* with a small, hackable codebase and a self-hosted UI, need unencrypted traffic (for example amateur radio use), or are learning how a LoRa node, SPIFFS web server, WebSocket bridge and dual-core firmware fit together.
- The projects are not interoperable. A HASTYviolet node cannot join a Meshtastic or Meshcore mesh, and vice versa, because they use different packet formats on the air.

## Supported ESP32 boards

| Board | MCU | LoRa chip | OLED | PlatformIO environment | Status |
|---|---|---|---|---|---|
| Heltec WiFi LoRa 32 **V2** (also sold as HiLetgo ESP32 LoRa V2) | ESP32 | SX1276 | SSD1306 128x64 | `heltec_wifi_lora_32_V2` (default) | Built and tested on hardware |
| Heltec WiFi LoRa 32 **V3** | ESP32-S3 | SX1262 | SSD1306 128x64 | `heltec_wifi_lora_32_V3` | Builds; not yet tested on hardware |

Notes:

- TX power limits come from the radio: **V2 2 to 20 dBm** (PA_BOOST), **V3 -9 to 22 dBm**. The UI's HIGH setting uses the board maximum.
- Pin assignments for both boards were checked against Heltec's datasheets and are in `src/HASviolet_boards.h`.
- The V3 radio setup (1.8 V TCXO supplied through DIO3, DIO2 as RF switch) comes from RadioLib/community conventions rather than the datasheet. If the V3 reports `LoRa radio not found`, check `HASV_LORA_TCXO_V` first.
- Other boards (TTGO, T-Beam, LoPy) were removed from the MVP and remain in git history.

Build and flash:

```
pio run -e heltec_wifi_lora_32_V2 -t uploadfs     # web UI files, only when data/ changes
pio run -e heltec_wifi_lora_32_V2 -t upload       # firmware
pio device monitor                                # serial console, 115200 baud
```

Use `heltec_wifi_lora_32_V3` instead for the V3. See `README.md` for serial-port setup.

## Web interface how-to

### Connect

1. Power the board. Wait for `INIT: COMPLETE` on the serial console (or the logo on the OLED).
2. Join the Wi-Fi network `HASviolet` (password `purple99`), or, if you configured a network in `HASviolet_config.h`, join that one and use the IP printed on the serial console (`300: WiFi client, IP ...`).
3. Open `http://192.168.4.1:8000` in a browser (use `http`, not `https`). The page connects to the WebSocket on port 8888 automatically.
4. The receive window shows the help screen. Click **HELP** at any time to bring it back.

### The screen

- **Tuner display (top left):** current frequency in MHz, plus spread factor (SF), coding rate (CR), bandwidth in kHz (BW) and channel name (CH).
- **Number pad (0 to 9):** selects one of ten channel presets.
- **RF controls:** RADIO, PA, MODE, BECN.
- **Macro buttons:** M1 to M4.
- **Receive window (center):** scrolling log of everything sent and received.
- **Message controls:** CALL, DEST, HEAD, HELP, LOG, RCLR, MCLR, SEND.
- **Transmit window (bottom):** where you type.

### Choose a channel

Press a number button to retune the radio. All nodes that should talk to each other must be on the same channel.

| Button | Channel | Frequency | Modem | Spread factor / bandwidth |
|---|---|---|---|---|
| 0, 1, 8 | HV0, HV1, HV8 | 911.25, 911.25, 911.30 MHz | MEDIUM | SF7, 125 kHz |
| 5, 9 | HV5, HV9 | 911.25, 911.30 MHz | FAST-SHRT | SF7, 500 kHz |
| 2, 6 | HV2, HV6 | 911.25, 911.30 MHz | SLOW-LNG1 | SF7, 31.25 kHz |
| 3, 7 | HV3, HV7 | 911.25, 911.30 MHz | SLOW-LNG2 | SF12, 125 kHz |
| 4 | HV4 | 911.25 MHz | SLOW-LNG3 | SF8, 125 kHz |

The board starts on the settings in `hasVIOLET.json` (HV1 equivalent, 911.25 MHz, MEDIUM). Channel presets can be edited in that file, followed by an `uploadfs`. Make sure the frequencies you use are legal for your region and licence.

### Send a message

Messages use the format `YOURCALL-SS>DESTCALL-SS|text`.

**With buttons:**

1. Click **HEAD**. The transmit window fills with your header, built from the `CONTACT` section of the JSON (for example `NOCALL-99>BEACON-99|`).
2. Type your text after the `|`, or click a macro (M1 to M4) to append it.
3. Click **SEND**.

**By typing:** type the full line yourself, for example `N0CALL-1>BEACON-99|hello`, then click **SEND**.

Sent messages appear as `TX: ...` and received ones as `RX: ...` with the signal strength (`|RSSI: -87`). Messages are limited to 254 bytes. Anything you send that does not start with a dot is transmitted as-is.

### Button reference

| Button | What it does |
|---|---|
| **PA** | Cycles TX power: LOW (7 dBm), MEDIUM (14 dBm), HIGH (board maximum: 20 dBm V2, 22 dBm V3). Applied to the radio immediately. |
| **RADIO** | Toggles a radio on/off indicator. *Placeholder: the firmware only acknowledges it.* |
| **MODE** | *Placeholder, no effect.* |
| **BECN** | Toggles a beacon indicator. *Placeholder: no beacon is transmitted yet.* |
| **M1 to M4** | Append the macro text (defaults: `QRZ`, `QSL`, `73`, `TEST TEST TEST`) to the transmit window. |
| **CALL** | Append your call sign and SSID. |
| **DEST** | Append the destination call sign and SSID. |
| **HEAD** | Append the full `CALL>DEST|` header. |
| **HELP** | Show the help screen in the receive window. |
| **LOG** | Toggles a logging indicator. |
| **RCLR** | Clear the receive window. |
| **MCLR** | Clear the transmit window. |
| **SEND** | Send the transmit window contents (or run a dot-command). |

### Dot-commands

Type these in the transmit window and click **SEND**. They change the browser's settings only and are lost when the page reloads. To make them permanent, edit `data/hasVIOLET.json` and run `pio run -t uploadfs`.

| Command | Effect |
|---|---|
| `.myCALL N0CALL-1` | Set your call sign and SSID (used by CALL and HEAD) |
| `.dstCALL BL0B-50` | Set the destination call sign and SSID (used by DEST and HEAD) |
| `.macro1 text` to `.macro4 text` | Change what M1 to M4 insert |

### Troubleshooting

- **Page loads but nothing sends or arrives:** open the browser developer console (Console tab) and look for `WSS:` lines. Port 8888 must be reachable and the page must be loaded over `http`.
- **No `RX:` lines:** confirm both nodes are on the same channel (frequency, bandwidth, SF, CR). Watch the serial console on the sender for `TX:` lines.
- **A message containing colons looks truncated:** this was fixed in the MVP. If you still see it, reload the page so you are not running a cached `hasVIOLET.js`.
- **Board will not start its AP:** the AP password must be at least 8 characters.
- **Serial console shows `LoRa radio not found, halting`:** the radio did not respond. Check you built the environment matching your board.
- **Linux `Permission denied` on the serial port:** `sudo usermod -aG dialout $USER`, then log out and back in.

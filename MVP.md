# MVP Session Summary

Work done on the `mvp` branch of HASTYviolet-ESP32 to support the Heltec WiFi LoRa 32 **V2** and **V3**, based on the lessons recorded in `BETA.md` of the `hasviolet-esp32` repo (`beta` branch).

## Goal

Bring the lessons learned from the HASviolet-ESP32 beta (a working V2-only rebuild of `alpha_0-3`) into HASTYviolet-ESP32, and extend the result to cover both Heltec V2 and V3 boards.

## Outcome

| Item | Result |
|---|---|
| Branch `mvp` created from `main` | Done, published by the owner |
| Heltec V2 build | Compiles; **tested on hardware and working** |
| Heltec V3 build | Compiles; **not yet tested on hardware** |
| Pin assignments | Checked against the Heltec V2 and V3 datasheets; no changes needed |
| README | Updated for V2/V3 |
| `USER.md` | Added: overview, comparison with Meshtastic/Meshcore, supported boards, web UI how-to |

## Lessons from BETA.md and how they were applied

| Lesson | Change in this repo |
|---|---|
| Nothing was version-pinned, so upstream releases broke builds | Platform and every library pinned in `platformio.ini` (espressif32 7.1.3, LoRa 0.8.0, ArduinoJson 7.4.3, ESPAsyncWebServer 3.12.1, AsyncTCP 3.5.0, WebSockets 2.7.3, Adafruit libraries; RadioLib 7.1.2 for V3) |
| Heltec env's `lib_deps` replaced the shared list | Board envs extend the shared list with `${env.lib_deps}` |
| Unused `Time.h`/`TimeLib.h` includes | Removed |
| Radio used from both cores | One `HasTRX` task on core 0 owns the radio and OLED; `loop()` on core 1 owns the WebSocket; data passes through `txQueue` and `rxQueue` |
| JSON loader read a nonexistent section, leaving the radio at 0 Hz | Reads `RADIO` and `CONTACT`, keeps defaults for missing fields; also fixed a `mySSID`/`myssid` key mismatch |
| Wi-Fi client mode could hang forever | Retries for about 10 seconds, then falls back to the AP and prints the IP |
| AP password under 8 characters means no AP | Default changed to `purple99`; documented |
| TX power not validated | Clamped per board (V2 2 to 20 dBm, V3 -9 to 22 dBm); HIGH uses the board maximum; changes apply to the radio immediately |
| Payload could overflow | Outgoing messages truncated to 254 bytes |
| Command parsing used `indexOf` | Uses `startsWith`, so text containing `TX:` cannot trigger a send |
| No radio meant silent failure | Reports on serial and the OLED, then halts |
| Favicon sent as `text/html` | Sent as `image/x-icon` |
| Web UI split incoming messages on every `:` | Splits on the first colon only, so full messages display |
| Real Wi-Fi credentials tracked in git | Credentials commented out in `HASviolet_config.h` (still present in git history) |
| Non-compiling TTGO/LoPy board paths | Removed; Heltec V2 and V3 only |
| Board library and pins mixed assumptions | Pins live in `HASviolet_boards.h` with `HASV_` names that do not collide with Arduino core names; board chosen by the PlatformIO environment |

## New work beyond BETA.md (V3 support)

- **Radio layer** (`src/HASviolet_radio.h`): one small API (`radioBegin`, `radioSend`, `radioReceive`, `radioSetTxPower`, `radioDump`) with two back ends, so `main.cpp` is the same for both boards.
  - V2: SX1276 through `sandeepmistry/LoRa`.
  - V3: SX1262 through RadioLib (private sync word, CRC on, DIO2 as RF switch, 1.8 V TCXO).
- **Board definitions** (`src/HASviolet_boards.h`): full pin maps and TX power limits for both boards.
- **OLED** (`src/HASviolet_OLED.h`): corrected to 128x64 with the explicit `0x3C` address, so it no longer falls back to `0x3D`; per-board SDA/SCL/RST pins; safe if the display is missing.
- **Board init**: Vext is switched on and the OLED reset pulsed before the OLED starts, on both boards.
- **Tuner command**: `SET:TUNER:` now reconfigures the radio inside the radio task (previously it only stored variables).
- **TX power commands**: queued to the radio task rather than set from the WebSocket core.

## Files changed

| File | Change |
|---|---|
| `platformio.ini` | Rewritten: pinned versions, V2 and V3 environments, no hardcoded serial port |
| `src/main.cpp` | Rewritten: queues, radio task, JSON loader, Wi-Fi fallback, command parsing |
| `src/HASviolet_radio.h` | New: radio abstraction for SX1276 and SX1262 |
| `src/HASviolet_boards.h` | Rewritten for V2 and V3 |
| `src/HASviolet_config.h` | Trimmed to user options; credentials removed; board selection moved to PlatformIO |
| `src/HASviolet_OLED.h` | Per-board pins, correct size and address, init result checked |
| `src/main.cpporg` | Removed (old backup, kept in git history) |
| `data/hasVIOLET.js` | RX handler shows the full message |
| `README.md` | Supported boards, pinned libraries, V3 commands, Wi-Fi fallback, boot log, single-core architecture, radio libraries |
| `USER.md` | New: overview, comparison with Meshtastic and Meshcore, boards, web UI how-to |

## Verification

- `pio run -e heltec_wifi_lora_32_V2` and `-e heltec_wifi_lora_32_V3` both build successfully.
- Flashed and run on a **Heltec V2**: confirmed working by the owner.
- Heltec **V2 and V3 datasheets** were downloaded and the pinout diagrams compared with the code. Every LoRa, OLED, Vext and LED pin matched.
- The V3 has **not** been flashed.

## Known gaps and follow-ups

- **V3 hardware test:** flash and verify. If it reports `LoRa radio not found`, check the TCXO voltage (`HASV_LORA_TCXO_V`) and DIO2 RF-switch setting first; neither is in the datasheet.
- **Credentials in history:** the Wi-Fi SSID and password committed before this work remain in git history; change that password.
- **Placeholder UI controls:** RADIO, MODE and BECN are acknowledged by the firmware but do nothing. Beacon transmission and a radio reset are unimplemented.
- **Dot-commands** (`.myCALL`, `.dstCALL`, `.macroN`) only change browser state and are lost on reload.
- **V3 serial:** the V3 uses native USB; serial output can drop across a reset, so the build sets `ARDUINO_USB_CDC_ON_BOOT=1`.
- **Cosmetic UI bug found, not fixed:** in `data/hasVIOLET.js` the `.myCALL` confirmation prints the SSID twice, and the `.macro2` to `.macro4` confirmations print wrong labels.

## Session notes

- `pio` is at `~/.platformio/penv/bin/pio` (not on `PATH`).
- A first push from the session failed for lack of credentials; the owner pushed the branch themselves.
- Mid-session `.vscode/extensions.json` was modified by PlatformIO and was reverted rather than committed.

# hasty-esther

HASviolet ESP32 is a (very much) work-in-progress LoRa transceiver project part of HASviolet.  

# Getting Started

The quickest way to get started with HASviolet ESP32 is to
* Buy a <a href="https://www.amazon.com/gp/product/B07WHRS2XG/" target="_blank">HiLetgo ESP32 LoRa v2 device</a>
* Download the [latest release](https://github.com/joecupano/hasty-esther/releases)
* Unzip the `hasviolet-esp32-0.X.X.zip` file
* Follow the included instructions for flashing it to your device

Alternatively, you can compile the firmware yourself by cloning this repo and following the [initial setup](#initial-setup-with-platformio) instructions.

# Architecture Model
HASviolet ESP32 is made up of a radio (LoRa transceiver), server, and client. 

- Server
    - an ESP32 device that runs Web/Websockets services for communications with the client
    - includeds LoRa libraries/functions for communications with the radio via the ESP32 <a href="http://iot-bits.com/esp32/esp32-spi-tutorial-part-1/" target=" blank">SPI interface</a>
    - runs code for radio control and communication with the Websockets and LoRa functions

- Radio
    - an RF hardware module (such as the <a href="https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276" target=" blank">sx1276</a>) capable of LoRa that is either embedded or external to the ESP32 device and connected via SPI

- Client
    - Web browser that runs the HASviolet ESP32 Web UI interface
    - Browser connects to Web service on ESP32 device and loads an <a href="https://github.com/joecupano/hasty-esther/blob/main/data/hasVIOLET_INDEX.html" target=" blank">HTML file</a> loaded with HTML5, CSS, and Javascript
    - On load, Javascript runs and connects to the Websocket service on the ESP32 device to establish a websocket session
    - Received data, data to be transmitted, and radio command are commuicated over the websocket session

# Initial Setup with PlatformIO

```
pip install -U platformio
```

Edit `platformio.ini` to suit your needs. If you are flashing a <a href="https://www.amazon.com/gp/product/B07WHRS2XG/" target="_blank">HiLetgo ESP32 LoRa v2 board</a> then you will probably not have to edit anything in `platformio.ini` but make sure `upload_port` is set to the correct device which may vary depending on your operating system and which other devices you have connected.

## Building firmware

To test that the firmware is compiling correctly without flashing it to a device, run
```
pio run
```

This will compile all libraries and main firmware files. The resulting binary is stored as `.pio/build/heltec_wifi_lora_32_v2/firmware.bin`

## Flashing firmware

### For dev boards

Connect your computer to the board using a usb cable,

In `platformio.ini` make sure `upload_port`, `upload_speed`, etc are correct for your device.

Then run:

```
pio run -t uploadfs
```
followed by :
```
pio run -t upload
```
 
By default, PlatformIO builds and flashes firmware for the <a href="https://www.amazon.com/gp/product/B07WHRS2XG/" target="_blank">HiLetgo ESP32 LoRa v2 board</a>. If you would like to build for another supported board, select the corresponding build environment. For example to build and flash the firmware and file system for the ESP32 TTG0 V1 board, use the following, 

```
pio run -e ttgo-lora32-v1 -t upload -t uploadfs

```

## Developing firmware

Some useful hints for developing this firmware using platformio are included here,
* It may be a good idea to delete the libdeps folder prior to rebuilding, as old, out-dated libraries could case conflits. To do this, `rm -rf .pio/libdeps`.
* If you would like to make changes to a specific library you can clone the library into the `firmware/lib` folder that is created after running `pio run` and then comment it out or remove it from the `lib_deps` list in the `platformio.ini` file.

## Building and uploading SPIFFS image

Build the binary for the SPIFFS and upload it to your board.
To only build:

```
pio run -t buildfs
```

To build and upload:

```
pio run -t uploadfs
```


# Testing Firmware  
Once the firmware and SPIFFS image has been successfully flashed, you can test it by logging into the node vai the WiFi network it is connected to for accessing the browser interface.  

You can also try connecting to the device via serial,  
```
pio device monitor
```

After pressing enter once, you should be greeted with following,
```
INIT: HASviolet ESP32
=====================================
 000: Setup running on core 1
 100: SPIFFs initialized
 300: WiFi CL initialized
 400: Webserver initialized
 500: Websockets initialized
 600: OLED Initialized
 700: HasTRX (LoRa) running on core 0
INIT: COMPLETE
=====================================

```
# Adding Libraries

If you're including new libraries in the firmware, for PlatformIO, you wil need to add them to `platformio.ini` under `lib_deps`.

# Creating Binary for Release

A full binary image can be created by reading the flash contents of a ESP32 that is flashed with the latest release of the firmware.
To do this, run the following command,
```
esptool.py -p /dev/ttyUSB0 -b 921600 read_flash 0 0x400000 esp_flash.bin
```

This can then be flashed to a new device like so,
```
esptool.py -p /dev/ttyUSB0 --baud 460800 write_flash 0x00000 esp_flash.bin
```

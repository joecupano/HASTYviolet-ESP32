//
//  HASviolet_boards.h
//
//  Pin and radio definitions for the supported boards. The board is chosen
//  by the platformio.ini environment (-DHASV_BOARD_V2 / -DHASV_BOARD_V3).
//  Names deliberately avoid the Arduino core's SCK/MISO/MOSI/SS/SDA/SCL.
//

#if defined(HASV_BOARD_V2)
/// Heltec WiFi LoRa 32 V2 - ESP32 + SX1276
/// pio: board = heltec_wifi_lora_32_V2
#define HASV_BOARD_NAME "Heltec V2"
#define HASV_LED 25
#define HASV_VEXT 21                // LOW = Vext on (powers the OLED)
/// LoRa (SX1276, sandeepmistry/LoRa)
#define HASV_LORA_SX1276
#define HASV_LORA_SCK 5
#define HASV_LORA_MISO 19
#define HASV_LORA_MOSI 27
#define HASV_LORA_CS 18
#define HASV_LORA_RST 14
#define HASV_LORA_DIO0 26
#define HASV_PABOOST true
#define HASV_TXPWR_MIN 2            // PA_BOOST range
#define HASV_TXPWR_MAX 20
/// OLED (SSD1306 128x64)
#define HASV_OLED_YES
#define HASV_OLED_SDA 4
#define HASV_OLED_SCL 15
#define HASV_OLED_RST 16
#define HASV_OLED_WIDTH 128
#define HASV_OLED_HEIGHT 64
#define HASV_OLED_ADDR 0x3C

#elif defined(HASV_BOARD_V3)
/// Heltec WiFi LoRa 32 V3 - ESP32-S3 + SX1262
/// pio: board = heltec_wifi_lora_32_V3
#define HASV_BOARD_NAME "Heltec V3"
#define HASV_LED 35
#define HASV_VEXT 36                // LOW = Vext on (powers the OLED)
/// LoRa (SX1262, RadioLib)
#define HASV_LORA_SX1262
#define HASV_LORA_SCK 9
#define HASV_LORA_MISO 11
#define HASV_LORA_MOSI 10
#define HASV_LORA_CS 8
#define HASV_LORA_RST 12
#define HASV_LORA_BUSY 13
#define HASV_LORA_DIO1 14
#define HASV_LORA_TCXO_V 1.8        // TCXO supplied via DIO3
#define HASV_TXPWR_MIN -9
#define HASV_TXPWR_MAX 22
/// OLED (SSD1306 128x64)
#define HASV_OLED_YES
#define HASV_OLED_SDA 17
#define HASV_OLED_SCL 18
#define HASV_OLED_RST 21
#define HASV_OLED_WIDTH 128
#define HASV_OLED_HEIGHT 64
#define HASV_OLED_ADDR 0x3C

#else
#error "No board defined: build with the heltec_wifi_lora_32_V2 or _V3 environment"
#endif

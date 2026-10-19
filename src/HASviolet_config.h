//
//  HASviolet_config.h
//
//  User configurable options. The board is selected in platformio.ini.
//

// Serial Console Baud Rate
#define HASV_SERIAL_BAUD 115200

// Webserver and Websocket ports
const uint16_t HASV_WEB_PORT = 8000;
const uint16_t HASV_WS_PORT = 8888;

// WiFi AP credentials (password must be at least 8 characters or the AP will not start)
#define HASV_WIFI_APSSID "HASviolet"
#define HASV_WIFI_APKEY "purple99"

// To join an existing WiFi network, uncomment and edit the next two lines.
// If the connection fails the node falls back to its own AP.
// Do not commit real credentials.
//#define HASV_WIFI_SSID "your-ssid"
//#define HASV_WIFI_KEY "your-password"
#define HASV_WIFI_POLL_TRIES 20
#define HASV_WIFI_POLL_DELAY 500    // ms

// Default LoRa modem settings (overridden by data/hasVIOLET.json)
#define HASV_RF_LORA_MODEMCONFIG "Bw125Cr45Sf128"
#define HASV_RF_LORA_FREQ_HZ 911250000
#define HASV_RF_LORA_BANDWIDTH 125000
#define HASV_RF_LORA_SF 7
#define HASV_RF_LORA_CR 8
#define HASV_RF_LORA_TXPWR 10

// Longest outgoing message in bytes
#define HASV_MAX_PAYLOAD 254

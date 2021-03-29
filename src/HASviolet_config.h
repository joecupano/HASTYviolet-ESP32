//
//  HASviolet_config.h
//
//     RELEASE:  20210328-0100
//


//
// BOARD SELECTION
//

#define TTGO_T_BEAM
// #define HELTEC_WIFI_LORA_32_V2
// #define LoPy
// #define LoPy4


//
// GENERAL
//

// Serial Console Baud Rate
#define HASV_SERIAL_BAUD 115200

// Asyncwebserver Port
// #define HASV_WEB_PORT 8000
const uint16_t HASV_WEB_PORT = 8000;
// Websocket Port
// #define HASV_WS_PORT 8888
const uint16_t HASV_WS_PORT = 8888;

// WiFi AP Creds
#define HASV_WIFI_APSSID "HASviolet"
#define HASV_WIFI_APKEY "purplehaste"
#define HASV_WIFI_APIP1 192
#define HASV_WIFI_APIP2 168
#define HASV_WIFI_APIP3 4
#define HASV_WIFI_APIP4 1

//Uncomment and edit the next two lines if connecting to existing WiFi network
#define HASV_WIFI_SSID "Excelsior"
#define HASV_WIFI_KEY "19144966520"


//
// LORA MODEM
//

#define HASV_RF_LORA_FREQ 911.25            // Frequency in MHz
#define HASV_RF_LORA_FREQ_HZ 911250000      // Frequency in Hz
#define HASV_RF_LORA_BW 125.0               // Bandwidth. Default is 125Khz
#define HASV_RF_LORA_SF 7                   // Spread Factor. Default is 7
#define HASV_RF_LORA_CR 8                   // Coding Rate. Default is 8
#define HASV_RF_LORA_TXPWR 20               // Transmit Power (7-22)
#define HASV_RF_LORA_GAIN 0                 // Antenna Gain correction


//
// GPS
//
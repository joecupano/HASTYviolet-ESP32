//
//  HASviolet_boards.h
//
//     RELEASE:  20210328-0100
//


//
// BOARDS
//

/// HELTEC_WIFI_LORA_32_V2
/// https://heltec.org/project/wifi-lora-32/
/// pio: board = heltec_wifi_lora_32_V2
///
#if defined(HELTEC_WIFI_LORA_32_V2)
#define HASV_LED 25          // GPI025 -- SX127x's LED
#define HASV_BAT 37
#define HASV_BAT_MULDIV 320 / 100

/// LoRa
#define HASV_LORA_SCK 5      // GPIO5  -- SX127x's SCK
#define HASV_LORA_MISO 19    // GPIO19 -- SX127x's LORA_MISO
#define HASV_LORA_MOSI 27    // GPIO27 -- SX127x's LORA_MOSI
#define HASV_LORA_CS 18      // GPIO18 -- SX127x's CS
#define HASV_LORA_RST 14     // GPIO14 -- SX127x's RESET
#define HASV_LORA_IRQ 26     // GPIO26 -- SX127x's IRQ(Interrupt Request)
#define HASV_PABOOST true
#define HASV_LORA_BAND 911250000  // Band setting ( 868E6,915E6 )
/// OLED
#define HASV_OLED_YES        //Uncomment Board or Peripheral features
#define HASV_OLED_SCL 15     // GPIO15 -- OLED_SCL
#define HASV_OLED_SDA 4      // GPIO4  -- OLED_SDA
#define HASV_OLED_RST 16     // GPIO16 -- OLED_RST
#define HASV_OLED_WIDTH 128
#define HASV_OLED_HEIGHT 32
/// VEXT
#define HASV_VEXT 21         // GPIO21 -- VEXT_CONTROL
/// aka
#define HASV_LED1 25         // aka LED
#define HASV_OLED_RESET 16   // aka OLED_RST
#define HASV_LORA_DIO0 26    // aka IRQ
#define HASV_OLED_CLOCK 15   // aka SCL
#define HASV_OLED_DATA 4     // aka SDA
#define HASV_OLED_RESET 16   // aka RST
/// actually missing
#define HASV_LORA_DIO1 -1


/// TTGO_T_BEAM  T22_V1.1 20191212
///
#elif defined(TTGO_T_BEAM)
#define HASV_LED 25 
#define HASV_BAT 35 
#define HASV_BAT_MULDIV 200 / 100 
/// LoRa
#define HASV_LORA_SCK 5      // GPIO5  -- SX1276 SCK
#define HASV_LORA_MISO 19    // GPIO19 -- SX1276 MISO
#define HASV_LORA_MOSI 27    // GPIO27 -- SX1276 MOSI
#define HASV_LORA_CS 18      // GPIO18 -- SX1276 CS
#define HASV_LORA_RST 14     // GPIO14 -- SX1276 RST
#define HASV_LORA_IRQ 26     // GPIO26 -- SX1276 IRQ (interrupt request)
#define HASV_LORA_DI0 26     // GPIO26 -- SX1276 IRQ aka DI0
/// OLED
#define HASV_OLED_NONE       
//#define HASV_OLED_CLOCK 22 
//#define HASV_OLED_DATA 21 
//#define HASV_OLED_RESET 16
//#define HASV_OLED_WIDTH 128
//#define HASV_OLED_HEIGHT 32
/// aka
//#define HASV_OLED_SCL 22     // aka OLED_CLOCK
//#define HASV_OLED_SDA 21     // aka OLED_DATA
//#define HASV_OLED_RST 16     // aka OLED_RESET
/// actually missing 
#define HASV_LORA_DIO1 -1


/// LoPy
/// https://docs.pycom.io/datasheets/development/lopy/
/// pio: board = lopy
///
#elif defined(LoPy)
#define HASV_ANT 16
/// LoRa
#define HASV_LORA_CS 17
#define HASV_LORA_DIO0 23
#define HASV_LORA_RST 18
/// SPI
#define HASV_LORA_SPI_SCK 5
#define HASV_LORA_SPI_MISO 19
#define HASV_LORA_SPI_MOSI 27
#define HASV_LORA_CLASS SX1272
/// OLED
#define HASV_OLED_NONE
/// actually missing
#define HASV_LORA_DIO1 -1


/// LoPy4
/// https://docs.pycom.io/datasheets/development/lopy4/
/// pio: board = lopy4
///
#elif defined(LoPy4)
#define HASV_ANT 16
/// LoRa
#define HASV_LORA_CS 18
#define HASV_LORA_DIO0 23
#define HASV_LORA_RST -1
/// SPI
#define HASV_LORA_SPI_SCK 5
#define HASV_LORA_SPI_MISO 19
#define HASV_LORA_SPI_MOSI 27
#define HASV_LORA_CLASS SX1276
/// OLED
#define HASV_OLED_NONE
/// actually missing
#define HASV_LORA_DIO1 -1

#endif  // Board definitions


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
// OLED
//

//#if defined(HASV_OLED_YES)                  // If no OLED then do nothing
//Adafruit_SSD1306 display(HASV_OLED_WIDTH, HASV_OLED_HEIGHT, &Wire, HASV_OLED_RESET);
//
//#else
//#endif


//
// WIFI
//


//
// BLUETOOTH
//


//
// GPS
//
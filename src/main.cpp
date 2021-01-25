//
//  HASviolet-ESP32.ino (DualCore)
//
//  INTRO
//
//  HASviolet node to be mansed via WebUI
//  This version connect to existing AP
//
//  20210124-0300
//     
//


//
// IMPORTED LIBRARIES
//

#include "Time.h"
#include "TimeLib.h"
#include "heltec.h"
#include "hvdn-logo.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "FreeRTOS.h"


//
// LEGEND
//

// Pin definition of WIFI LoRa 32 v2.0
// HelTec AutoMation 2019 support@heltec.cn
//
// LoRa
//#define SCK     5    // GPIO5  -- SX127x's SCK
//#define MISO    19   // GPIO19 -- SX127x's MISO
//#define MOSI    27   // GPIO27 -- SX127x's MOSI
//#define SS      18   // GPIO18 -- SX127x's CS
//#define RST     14   // GPIO14 -- SX127x's RESET
//#define DI0     26   // GPIO26 -- SX127x's IRQ(Interrupt Request)
//#define PABOOST true
#define BAND    911250000  // you can set band here directly, ( 868E6,915E6 )
//
// OLED
// define SCL  15  // GPIO15 -- OLED
// define SDA   4  // GPIO4  -- OLED
// define RST  16  // GPIO16 -- OLED
//
// VEXT
// define VEXT 21  // GPIO21 -- VEXT


//
// VARIABLES
//

// Set WiFi Creds
const char* WIFI_SSID = "Excelsior";         // SSID of WiFi 
const char* WIFI_KEY = "19144966520";        // Password of WiFi

// Set WiFi Mode
bool wifi_ap = false;                        // False = Setup as Wifi client, True = Setup as WiFi AP

// Set WebAuthh
const char* wwwUSERNAME;                     // Webpage Login
const char* wwwAUTHPASS;                     // Webpage password

//Set Servers
const uint16_t WEB_PORT = 80;
const uint16_t WS_PORT = 8000;

//Task handles
TaskHandle_t TaskTRX;
TaskHandle_t TaskWebsox;


// Configure LoRa and Appstack
// byte localaddressLORA = 0xBB;             // LoRa address of this device (irrelevant)
byte destinationLORA = 0xFF;                 // LoRa destination to send to (broadcast default)
int frequency = 911250000;                   // HASviolet default settings for safe LoRa module init
int spreadfactor = 7;
int codingrate4 = 8;
int bandwidth = 125000;
int txpwr = 11;

String mycall = "NOCALL";
String myssid = "00";
String dstcall = "BEACON";                   // Destination Call ( BEACON )
String dstssid = "99";                       // Destination SSID ( 99 )
String mybeacon = "QRZ? QRZ? QRZ?";          // Baacon Message
String lastMsgRX = "None";                   // Last received message
//String lastMsgTX = "None";                   // Last transmitted 
//String lastWebsRX = "None";                  // Last received Websocket message
//String lastWebsTX = "None";                  // Last transmitted Websocket message

//String wsmessage = "";
String payloadS = "";
//String payloadMSG = "";

// Create Servers
AsyncWebServer server(WEB_PORT);
WebSocketsServer webSocket(WS_PORT);


//
// FUNCTIONS
//

/// Main Tasks
void HasWebsox(void *pvParameters) {
  Serial.print("800: Task *HasWebSox* on core ");
  Serial.println(xPortGetCoreID());
  while (true) {
    webSocket.loop();
  }
}

void HasTRX(void *pvParameters) {
  // Initialize LoRa
  LoRa.setSyncWord(0xFF);                                 // Set for LoRa Broadcast
  LoRa.setFrequency(frequency);
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setCodingRate4(codingrate4);
  LoRa.setSpreadingFactor(spreadfactor);
  LoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
  // register the receive callback
  // LoRa.onReceive(onReceiveLORA);
  LoRa.receive();
  Serial.print("700: HasTRX (LoRa) running on core "); 
  Serial.println(xPortGetCoreID());
  while (true) {
    delay(10);
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      lastMsgRX = "";
      for (int i = 0; i < packetSize; i++)
        lastMsgRX = lastMsgRX + ((char)LoRa.read());
      lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(LoRa.packetRssi());
      Serial.println("2000: Received LoRa packet");
      Serial.println(lastMsgRX);
      webSocket.broadcastTXT(lastMsgRX);
    }
  }
}


/// LoRa
void sendLORA(String outgoing)
{
  LoRa.beginPacket();                        // start packet
  LoRa.write(destinationLORA);               // add destination address
  //LoRa.write(localaddressLORA);              // add sender address
  //LoRa.write(msgCount);                      // add message ID
  //LoRa.write(outgoing.length());             // add payload length
  LoRa.print(outgoing);                      // add payload
  LoRa.endPacket();                          // finish packet and send it
  //OLEDme("Sending ...",1000);
  Serial.println("3000: Sent LoRa message");
  Serial.println(outgoing);
}

void onReceiveLORA(int packetSize)
{
  // read packet
  lastMsgRX = "";
  for (int i = 0; i < packetSize; i++)
  {
    lastMsgRX = lastMsgRX + ((char)LoRa.read());
  }
  lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(LoRa.packetRssi());
  Serial.println("2000: Received LoRa packet");
  Serial.println(lastMsgRX);
  webSocket.broadcastTXT(lastMsgRX);
}

/// WebServer
void onIndexRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET_INDEX.html", "text/html");
}

void onJsonRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.json", "application/json");
}

/// WebSockets
bool isEqual(uint8_t * payload, char * message) {
    return strcmp((char *)payload, message) == 0;
}

void onWebSocketEvent(uint8_t clientID, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      String payloadS = (char*)payload;
      
      //RADIO
      if (payloadS == "SET:RADIO:ON") {
        // turn me on
        webSocket.sendTXT(clientID, "ACK:RADIO:ON");
      } else if (payloadS == "SET:RADIO:OFF") {
          // turn me on
          webSocket.sendTXT(clientID, "ACK:RADIO:OFF");
      }
      
      // DUMP LORA
      if (payloadS == "GET:LORA") {
        // dump lora config
        LoRa.dumpRegisters(Serial);
        webSocket.sendTXT(clientID, "ACK:GET:LORA");
      }
      
      //BEACON
      if (payloadS == "SET:BEACON:ON") {
        // turn me on
        webSocket.sendTXT(clientID, "ACK:BEACON:ON");
      } else if (payloadS == "SET:BEACON:OFF") {
        // turn me on
        webSocket.sendTXT(clientID, "ACK:BEACON:OFF");
      }
      
      //TXPWR
      if (payloadS == "SET:TXPWR:LOW") {
        // set me low
        webSocket.sendTXT(clientID, "ACK:TXPWR:LOW");
      } else if (payloadS == "SET:TXPWR:MEDIUM") {
        // set me medium
        webSocket.sendTXT(clientID, "ACK:TXPWR:MEDIUM");
      } else if (payloadS == "SET:TXPWR:HIGH") {
        // set me high
        webSocket.sendTXT(clientID, "ACK:TXPWR:HIGH");
      }
      
      //CHANNEL
      if (payloadS.indexOf("SET:CHANNEL:") >= 0) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:SET:CHANNEL:");
      }
      
      //TX
      if (payloadS.indexOf("TX:") >= 0) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:" + payloadS);
        payloadS.replace("TX:","");
        sendLORA(payloadS);
      }
      
      //RX
      if (payloadS.indexOf("RRRRX:") >= 0) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:" + payloadS);
      }
    }
}

/// OLED DISPLAY FUNCTIONS
void logo()
{
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,hvdnimg_width,hvdnimg_height,hvdnimg_bits);
  Heltec.display->display();
  delay(1500);
  Heltec.display->clear();
}

/// Display myOLEDmsg on OLED
void OLEDme(String myOLEDmsg, int oledTIME)
{
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 15, myOLEDmsg);
  Heltec.display->display();
  delay(oledTIME);
  Heltec.display->clear();
}

/// SETUP FUNCTIONS
void initSerial() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();
  Serial.println();
}

void initSPIFFS() {
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("ERR: SPIFFS mounting");
    return;
  }
  Serial.println("100: SPIFFs initialized");
}

void loadJsonFile() {
  File configFile = SPIFFS.open("/hasVIOLET.json", "r");
  DynamicJsonDocument doc(1024);
  // The filter: it contains "true" for each value we want to keep
  StaticJsonDocument<500> filter;
  filter["CURRENT"] = true;
  if(!configFile){
    Serial.println("Failed to open hasVIOLET.json for reading");
    return;
  } else {
    DeserializationError error = deserializeJson(doc, configFile, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print("Error parsing hasVIOLET.json [");
      Serial.print(error.c_str());
      Serial.println("]");
    }
    // Place JSON into Variables
    wifi_ap = bool(doc["CURRENT"]["wifi_ap"]);
    //WIFI_SSID = (const char*)doc["CURRENT"]["wifi_ssid"];
    //WIFI_KEY = (const char*)doc["CURRENT"]["wifi_key"];
    mycall = (const char*)doc["CURRENT"]["mycall"];
    myssid = (const char*)doc["CURRENT"]["mySSID"];
    dstcall = (const char*)doc["CURRENT"]["dstcall"];
    dstssid = (const char*)doc["CURRENT"]["dstssid"];
    mybeacon = (const char*)doc["CURRENT"]["mybeacon"];
    frequency = int(doc["CURRENT"]["frequency"]);
    spreadfactor = int(doc["CURRENT"]["spreadfactor"]);
    codingrate4 = int(doc["CURRENT"]["codingrate4"]);
    bandwidth = int(doc["CURRENT"]["bandwidth"]);
    txpwr = int(doc["CURRENT"]["txpwr"]);
    configFile.close();
  }
  Serial.println("200: JSON loaded");
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.println("300: WiFi CL initialized");
  //Serial.println("300: WiFi MAC " + WiFi.macAddress());
  //Serial.println("310: WiFi IP  " + WiFi.localIP());
  //Serial.println("320: WiFi CL initialized");
}

void initWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_KEY);
  IPAddress ip = WiFi.softAPIP();
  Serial.println("300: WiFi MAC " + WiFi.macAddress());
  Serial.println("310: WiFi IP  " + ip);
  Serial.println("320: WiFi AP initialized");
}

void initWebServer() {
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/hasVIOLET.json", HTTP_GET, onJsonRequest);
  server.begin();
  Serial.println("400: Webserver initialized");
}

void initWebSockets() {
  // Websocket Handler
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("500: Websockets initialized");
}

void initOLED() {
  // Start OLED 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10); Serial.println("600: OLED Initialized");
}

//
// SETUP
//

void setup() {
 
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  initSerial();
  Serial.println("INIT: HASviolet ESP32");
  Serial.println("===============================");
  Serial.print("000: Setup running on core ");
  Serial.println(xPortGetCoreID());
  initSPIFFS();
  //loadJsonFile();
  initWiFi();
  initWebServer();
  initWebSockets();
  initOLED();
  logo();
  //initLORA();


  // Create core assignments : HasTRX on Core 0
  // hasWebsox on Core 1 butno need to assign since Loop on that core by default
  xTaskCreatePinnedToCore(HasTRX, "HasTRX", 10000, NULL, 1, &TaskTRX, 0);
  delay(500);
  //xTaskCreatePinnedToCore(HasWebsox, "HasWebsox", 10000, NULL, 1, &TaskWebsox, 1);
  //delay(500);

  Serial.println("===============================");
  Serial.print("999: HASviolet available");
  Serial.println();
  Serial.println();
}

//
// MAIN
//

void loop(){
  Serial.print("HasWebSox running in Loop on core ");
  Serial.println(xPortGetCoreID());
  Serial.println();
  while (true) 
  {
    webSocket.loop();
  }
}

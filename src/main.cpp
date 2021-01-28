//
//  HASviolet-ESP32.ino (DualCore)
//
//  INTRO
//
//  HASviolet node to be mansed via WebUI
//  This version connect to existing AP
//
//  20210128-0200
//     
//


//
// LIBRARIES
//

#include "Time.h"
#include "TimeLib.h"
#include "heltec.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "FreeRTOS.h"
#include "HASviolet_boards.h"
#include "HASviolet_channels.h"
#include "HASviolet_config.h"
#include "HASviolet_creds.h"
#include "HASviolet_logo.h"



//
// DEFINES
//
#define HELTEC_WIFI_LORA_32_V2            // Board selected
#define WIFI_POLL_DELAY 500
#define WIFI_POLL_TRIES 20
#define INDEX_FILE "hasVIOLET_index.htm"
#define BAND 911250000                    // you can set band here directly, ( 868E6,915E6 )
#define HVDNCH1                           // HVDN LoRa Channel


//
// VARIABLES
//

boolean fMyTask_Restart;                  // A trigger to restart LoRa Receiver after transmit

//Set Servers
const uint16_t WEB_PORT = 80;
const uint16_t WS_PORT = 8000;

//Task handles
TaskHandle_t TaskTRX;
TaskHandle_t TaskWebsox;

// Configure LoRa
// byte localaddressLORA = 0xBB;          // LoRa address of this device (irrelevant)
byte destinationLORA = 0xFF;              // LoRa destination to send to (broadcast default)

int frequency = 911250000;                // HASviolet default settings for safe LoRa module init
int spreadfactor = 7;
int codingrate4 = 8;
int bandwidth = 125000;
int txpwr = TXPWR;

String mycall = "NOCALL";
String myssid = "00";
String dstcall = "BEACON";                // Destination Call ( BEACON )
String dstssid = "99";                    // Destination SSID ( 99 )
String mybeacon = "QRZ? QRZ? QRZ?";       // Baacon Message
String lastMsgRX = " ";                   // Last received message
String payloadS = "";
//String lastMsgTX = "None";              // Last transmitted 
//String lastWebsRX = "None";             // Last received Websocket message
//String lastWebsTX = "None";             // Last transmitted Websocket message

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
  while (true) {
    fMyTask_Restart = false;
    // Initialize LoRa
    LoRa.setSyncWord(0xFF);                 // Set for LoRa Broadcast
    LoRa.setFrequency(frequency);
    LoRa.setSignalBandwidth(bandwidth);
    LoRa.setCodingRate4(codingrate4);
    LoRa.setSpreadingFactor(spreadfactor);
    LoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
    // register the receive callback
    // LoRa.onReceive(onReceiveLORA);
    LoRa.receive();
    Serial.print("CPU("); 
    Serial.print(xPortGetCoreID());
    Serial.println("): Task (re)start - HasTRX (LoRa)"); 
    while (!fMyTask_Restart) {
      delay(5);
      // try to parse packet
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        lastMsgRX = "";
        for (int i = 0; i < packetSize; i++)
          lastMsgRX = lastMsgRX + ((char)LoRa.read());
        lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(LoRa.packetRssi());
        Serial.println(lastMsgRX);
        webSocket.broadcastTXT(lastMsgRX);
      }
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
void OLEDme(String myOLEDmsg)
{
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 15, myOLEDmsg);
  Heltec.display->display();
  //delay(oledTIME);
  //Heltec.display->clear();
}

/// LoRa
void sendLORA(String outgoing)
{
  LoRa.beginPacket();                     // start packet
  LoRa.write(destinationLORA);            // add destination address
  //LoRa.write(localaddressLORA);         // add sender address
  //LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                   // add payload
  LoRa.endPacket();                       // finish packet and send it
  #ifdef HAS_OLED
  OLEDme(outgoing);
  #endif
  Serial.print("TX:");
  Serial.println(outgoing);
  fMyTask_Restart = true;
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
  Serial.print("RX:");
  Serial.println(lastMsgRX);
  #ifdef HAS_OLED
  OLEDme(lastMsgRX);
  #endif
  webSocket.broadcastTXT(lastMsgRX);
}

/// WebServer
void onFavRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/favicon.ico", "text/html");
}

void onIndexRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET_INDEX.html", "text/html");
}

void onCssRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.css", "text/css");
}

void onJsRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.js", "text/javascript");
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
    }
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
    Serial.println(" ERR: SPIFFS mounting");
    return;
  }
  Serial.println(" 100: SPIFFs initialized");
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
  Serial.println(" 200: JSON loaded");
}

void initWiFi() {
  #ifdef WIFI_SSID
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.println(" 300: WiFi CL initialized");
  //Serial.println("310: WiFi IP  " + WiFi.localIP());
  //Serial.println("320: WiFi CL initialized");
  #endif

  if (WiFi.status() != WL_CONNECTED) 
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_APSSID, WIFI_APKEY);
    IPAddress ip = WiFi.softAPIP();
    Serial.println(" 300: WiFi AP initialized");
    //Serial.println(" 310: WiFi IP  " + ip);
    //Serial.println(" 320: WiFi AP initialized");
  }
  
}

void initWebServer() {
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.on("/hasVIOLET.css", HTTP_GET, onCssRequest);
  server.on("/hasVIOLET.js", HTTP_GET, onJsRequest);
  server.on("/hasVIOLET.json", HTTP_GET, onJsonRequest);
  server.begin();
  Serial.println(" 400: Webserver initialized");
}

void initWebSockets() {
  // Websocket Handler
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println(" 500: Websockets initialized");
}

void initOLED() {
  // Start OLED 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10); Serial.println(" 600: OLED Initialized");
}

//
// SETUP
//

void setup() {
  #ifdef HELTEC_WIFI_LORA_32_V2 
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  #endif
  initSerial();
  Serial.println("INIT: HASviolet ESP32");
  Serial.println("=====================================");
  Serial.print(" 000: Setup running on core ");
  Serial.println(xPortGetCoreID());
  initSPIFFS();
  //loadJsonFile();
  initWiFi();
  initWebServer();
  initWebSockets();
  #ifdef HAS_OLED
  initOLED();
  #endif
  logo();
  Serial.println("INIT: COMPLETE");
  Serial.println("=====================================");
  Serial.println();
  
  // Create core assignments : HasTRX on Core 0
  // hasWebsox on Core 1 butno need to assign since Loop on that core by default
  xTaskCreatePinnedToCore(HasTRX, "HasTRX", 10000, NULL, 1, &TaskTRX, 0);
  delay(500);
  //xTaskCreatePinnedToCore(HasWebsox, "HasWebsox", 10000, NULL, 1, &TaskWebsox, 1);
  //delay(500);

  Serial.print("CPU("); 
  Serial.print(xPortGetCoreID());
  Serial.println("): Task (re)start - WiFi, Web, WebSox"); 
}

//
// MAIN
//

void loop(){
  Serial.println("  LOOP: Started");
  while (true) 
  {
    webSocket.loop();
  }
}

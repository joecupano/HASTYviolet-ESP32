//
//  HASviolet-ESP32
//
//
//  20210320-1700
//     
//


//
// LIBRARIES
//

#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include "Time.h"
#include "TimeLib.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "LoRa.h"
#include "HASviolet_config.h"
#include "HASviolet_boards.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "FreeRTOS.h"
#ifdef HASV_OLED_YES
#include "HASviolet_OLED.h"
#endif

//
// VARIABLES
//

// Configure LoRa
// byte localaddressLORA = 0xBB;                 // LoRa address of this device (irrelevant)
byte destinationLORA = 0xFF;                     // LoRa destination to send to (broadcast default)

// Default settings before HASviolet.json load
String channel = "HV1";                          // Channel
String modemconfig = HASV_RF_LORA_MODEMCONFIG;   // Modemstring Radiohead
int frequency = HASV_RF_LORA_FREQ_HZ;            // HASviolet default settings for safe LoRa module init
int spreadfactor = HASV_RF_LORA_SF;
int codingrate4 = HASV_RF_LORA_CR;
int bandwidth = HASV_RF_LORA_BANDWIDTH;
int txpwr = HASV_RF_LORA_TXPWR;
String mycall = "NOCALL";
String myssid = "00";
String mybeacon = "QRZ? QRZ? QRZ?";              // Baacon Message
String dstcall = "BEACON";                       // Destination Call ( BEACON )
String dstssid = "99";                           // Destination SSID ( 99 )

String lastMsgRX = " ";                          // Last received message
String payloadS = "";                            // LoRa message App layer payload
String lastMsgTX = " ";                          // Last transmitted 

// Create Servers
AsyncWebServer server(HASV_WEB_PORT);
WebSocketsServer webSocket(HASV_WS_PORT);


//
// FUNCTIONS
//

/// Split String

String getSubString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void sendLORA(String outgoing) {
  LoRa.beginPacket();                            // start packet
  //LoRa.write(destinationLORA);                 // add destination address
  //LoRa.write(localaddressLORA);                // add sender address
  //LoRa.write(outgoing.length());               // add payload length
  LoRa.print(outgoing);                          // add payload
  LoRa.endPacket();                              // finish packet and send it
  #ifdef HASV_OLED_YES
  OLEDme(outgoing);
  #endif
  Serial.print("TX:");
  Serial.println(outgoing);
  lastMsgTX = outgoing;                          // Record last message sent
}

void onReceiveLORA(int packetSize) {
  if (packetSize == 0) return;                   // if there's no packet, return
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  lastMsgRX = "";
  lastMsgRX = "RX:" + incoming + "|RSSI: " + String(LoRa.packetRssi());
  Serial.print("RX:");
  Serial.println(lastMsgRX);
  #ifdef HASV_OLED_YES
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
      if (payloadS == "SET:RADIO:RESET") {
        // Reset Radio - HASTRX task
        webSocket.sendTXT(clientID, "ACK:RADIO:RESET");
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
        //vTaskResume(TaskBeacon);                         // Add a beacon vtask to Core 1
      } else if (payloadS == "SET:BEACON:OFF") {
        // turn me off
        webSocket.sendTXT(clientID, "ACK:BEACON:OFF");
        //vTaskSuspend(TaskBeacon);                         // Kill beacon vtask to Core 1
      }
      
      //TXPWR
      if (payloadS == "SET:TXPWR:LOW") {
        // set me low
        txpwr = 7;
        webSocket.sendTXT(clientID, "ACK:TXPWR:LOW");
      } else if (payloadS == "SET:TXPWR:MEDIUM") {
        // set me medium
        txpwr = 14;
        webSocket.sendTXT(clientID, "ACK:TXPWR:MEDIUM");
      } else if (payloadS == "SET:TXPWR:HIGH") {
        // set me high
        txpwr = 21;
        webSocket.sendTXT(clientID, "ACK:TXPWR:HIGH");
      }
      
      //CHANNEL NEW
      if (payloadS.indexOf("SET:TUNER:") >= 0) {
        webSocket.sendTXT(clientID, "ACK:SET:TUNER:");
        String localjunk = getSubString(payloadS,':',0);
        String localjunk2 = getSubString(payloadS,':',1);
        String localmodemconfig = getSubString(payloadS,':',2);
        String localfrequency = getSubString(payloadS,':',3);
        modemconfig = localmodemconfig;
        frequency = localfrequency.toInt();
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
    channel	= (const char*)doc["RADIO"]["channel"];
		modemconfig = (const char*)doc["RADIO"]["modemconfig"];
    frequency = int(doc["RADIO"]["frequency"]);
    spreadfactor = int(doc["RADIO"]["spreadfactor"]);
    codingrate4 = int(doc["RADIO"]["codingrate4"]);
    bandwidth = int(doc["RADIO"]["bandwidth"]);
    txpwr = int(doc["RADIO"]["txpwr"]);
    mycall = (const char*)doc["CONTACT"]["mycall"];
    myssid = (const char*)doc["CONTACT"]["mySSID"];
    mybeacon = (const char*)doc["CONTACT"]["mybeacon"];
    dstcall = (const char*)doc["CONTACT"]["dstcall"];
    dstssid = (const char*)doc["CONTACT"]["dstssid"];
    configFile.close();
  }
  Serial.println(" 200: JSON loaded");
}

void initWiFi() {
  #ifdef HASV_WIFI_SSID
  WiFi.mode(WIFI_STA);
  WiFi.begin(HASV_WIFI_SSID, HASV_WIFI_KEY);
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
    WiFi.softAP(HASV_WIFI_APSSID, HASV_WIFI_APKEY);
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

void initLoRa() {
  SPI.begin(HASV_LORA_SCK, HASV_LORA_MISO, HASV_LORA_MOSI, HASV_LORA_CS); 
  LoRa.setPins(HASV_LORA_CS, HASV_LORA_RST, HASV_LORA_IRQ);         
  LoRa.begin(frequency);
  //LoRa.setSyncWord(0xFF);                       // Set for LoRa Broadcast
  //LoRa.disableCrc();
  LoRa.setSpreadingFactor(spreadfactor);          // ranges from 6-12, default 7
  LoRa.setFrequency(frequency);                   // default 911250000 Hz
  LoRa.setCodingRate4(codingrate4);               // default is 8
  LoRa.setSignalBandwidth(bandwidth);             // default is 125000 Hz
  #ifdef HASV_PABOOST
  LoRa.setTxPower(txpwr, HASV_PABOOST);
  #else
  LoRa.setTxPower(txpwr);
  #endif
  Serial.println(" 600: LoRa initialized");
  if (modemconfig == "Bw125Cr45Sf128") {
      LoRa.setSignalBandwidth(125000);
      LoRa.setSpreadingFactor(7);
      LoRa.setCodingRate4(8);
    }
    else if (modemconfig == "Bw500Cr45Sf128") {
      LoRa.setSignalBandwidth(500000);
      LoRa.setSpreadingFactor(7);
      LoRa.setCodingRate4(5);
    }
    else if (modemconfig == "Bw31_25Cr48Sf512") {
      LoRa.setSignalBandwidth(31250);
      LoRa.setSpreadingFactor(7);
      LoRa.setCodingRate4(8);
    }
    else if (modemconfig ==  "Bw125Cr48Sf4096") {
      LoRa.setSignalBandwidth(125000);
      LoRa.setSpreadingFactor(12);
      LoRa.setCodingRate4(8);
    }
    else if (modemconfig ==  "Bw125Cr45Sf2048") {
      LoRa.setSignalBandwidth(125000);
      LoRa.setSpreadingFactor(8);
      LoRa.setCodingRate4(5);
    }
    else {
      LoRa.setSignalBandwidth(125000);
      LoRa.setSpreadingFactor(7);
      LoRa.setCodingRate4(8);
  }
}

//
// SETUP
//

void setup() {
  initSerial();
  Serial.println("INIT: HASviolet ESP32");
  Serial.println("=====================================");
  Serial.print(" 000: Setup running on core ");
  Serial.println(xPortGetCoreID());
  initSPIFFS();
  loadJsonFile();
  initWiFi();
  initWebServer();
  initWebSockets();
  initLoRa();
  #ifdef HASV_OLED_YES
  initOLED();
  logo();
  #endif
  Serial.println("INIT: COMPLETE");
  Serial.println("=====================================");
  Serial.println();

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
    onReceiveLORA(LoRa.parsePacket());
  }
}

//
//  HASviolet-ESP32.ino
//
//  INTRO
//
//  HASviolet node to be mansed via WebUI
//  This version connect to existing AP
//
//  20210104-0200
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
#include "streetcred.h"


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
#define BAND    911E6  // you can set band here directly, ( 868E6,915E6 )
//#define PABOOST true
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

// Set WiFi 
bool wifi_ap = false;                        // False = Setup as Wifi client, True = Setup as WiFi AP

//Set Servers
const uint16_t WEB_PORT = 80;
const uint16_t WS_PORT = 8000;

// Configure LoRa and Appstack
byte localaddressLORA = 0xBB;                // LoRa address of this device (irrelevant)
byte destinationLORA = 0xFF;                 // LoRa destination to send to (broadcast default)
byte msgCount = 0;                           // count of outgoing messages
//long lastSendTime = 0;                     // last send time
//int interval = 2000;                       // interval between sends
int frequency = 911250000;                   // HASviolet default settings for safe LoRa module init
int spread = 7;
int codingrate4 = 8;
int bandwidth = 312500;
int txpwr = 5;

String mycall = "NOCALL";
String myssid = "00";
String dstcall = "BEACON";                   // Destination Call ( BEACON )
String dstssid = "99";                       // Destination SSID ( 99 )
String mybeacon = "QRZ? QRZ? QRZ?";          // Baacon Message
String lastmsgrx = "None";                   // Last received message
String lastmsgtx = "None";                   // Last transmitted 
String lastwebsrx = "None";                  // Last received Websocket message
String lastwebstx = "None";                  // Last transmitted Websocket message

String wsmessage = "";

// Create Servers
AsyncWebServer server(WEB_PORT);
WebSocketsServer webSocket(WS_PORT);


//
// FUNCTIONS
//

/// WebServer
void onIndexRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET_ALLINONE.html", "text/html");
}

void onJsonRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.json", "application/json");
}

void onStyleRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.css", "text/css");
}

void onScriptRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET_NOSSL.js", "text/javascript");
}

void onPageNotFound(AsyncWebServerRequest * request) {
    request->send(404, "text/plain", "Not found");
}

///WebSockets
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
      if (payloadS.indexOf("SET:CHANNEL:")) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:SET:CHANNEL:");
      }
      //TX
      if (payloadS.indexOf("TX:")) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:" + payloadS);
      }
      //RX
            if (payloadS.indexOf("TX:")) {
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
  Serial.println("000: HASviolet ESP32");
  Serial.println("===============================");
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
    wifi_ssid = (const char*)doc["CURRENT"]["wifi_ssid"];
    wifi_key = (const char*)doc["CURRENT"]["wifi_key"];
    mycall = (const char*)doc["CURRENT"]["mycall"];
    myssid = (const char*)doc["CURRENT"]["mySSID"];
    dstcall = (const char*)doc["CURRENT"]["dstcall"];
    dstssid = (const char*)doc["CURRENT"]["dstssid"];
    mybeacon = (const char*)doc["CURRENT"]["mybeacon"];
    frequency = int(doc["CURRENT"]["frequency"]);
    spread = int(doc["CURRENT"]["spread"]);
    codingrate4 = int(doc["CURRENT"]["codingrate4"]);
    bandwidth = int(doc["CURRENT"]["bandwidth"]);
    txpwr = int(doc["CURRENT"]["txpwr"]);
    configFile.close();
  }
  Serial.println("200: JSON loaded");
}

void initWiFi() {
  WiFi.begin(wifi_ssid, wifi_key);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("300: WiFi MAC " + WiFi.macAddress());
  Serial.println("310: WiFi IP  " + WiFi.localIP());
  Serial.println("320: WiFi CL initialized");
}

void initWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(wifi_ssid, wifi_key);
  IPAddress ip = WiFi.softAPIP();
  Serial.println("300: WiFi MAC " + WiFi.macAddress());
  Serial.println("310: WiFi IP  " + ip);
  Serial.println("320: WiFi AP initialized");
}

void initWebServer() {
  server.on("/",          HTTP_GET, onIndexRequest);
  server.on("/index.css", HTTP_GET, onStyleRequest);
  server.on("/index.js",  HTTP_GET, onScriptRequest);
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

void initLORA() {
  // Initialize LoRa
  //LoRa.onReceive(receiveLORA);
  //LoRa.receive();
  LoRa.setFrequency(frequency);
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setCodingRate4(codingrate4);
  LoRa.setSpreadingFactor(spread);
  LoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
  //LoRa.setSyncWord(0x12);   //0x34
  //LoRa.setPreambleLength(8);
  Serial.println("700: LoRa initialized"); 
}

void sendLORA(String outgoing)
{
  LoRa.beginPacket();                        // start packet
  LoRa.write(destinationLORA);               // add destination address
  LoRa.write(localaddressLORA);              // add sender address
  LoRa.write(msgCount);                      // add message ID
  LoRa.write(outgoing.length());             // add payload length
  LoRa.print(outgoing);                      // add payload
  LoRa.endPacket();                          // finish packet and send it
  Serial.println("3000: Sent LoRa packet");
}

void receiveLORA(int packetSize)
{
  // received a packet
  Serial.println("2000: Received LoRa packet");
  if (packetSize == 0) return;               // if there's no packet, return
  // read packet header bytes:
  int recipient = LoRa.read();               // recipient address
  byte sender = LoRa.read();                 // sender address
  byte incomingMsgId = LoRa.read();          // incoming msg ID
  byte incomingLength = LoRa.read();         // incoming msg length
  String incoming = "";                      // payload of packet
  while (LoRa.available())                   // can't use readString() in callback
  {
    incoming += (char)LoRa.read();           // add bytes one by one
  }
  if (incomingLength != incoming.length())   // check length for error
  {
    Serial.println("error: message length does not match length");
    return;                                  // skip rest of function
  }
  lastmsgrx = incoming;                      // Save as last rx message
  OLEDme(incoming, 1000);
  Serial.println();
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}

//
// SETUP
//

void setup() {
  initSerial();
  initSPIFFS();
  loadJsonFile();
  delay(1000);
  //initWiFi();
  WiFi.begin("Excelsior", "19144966520");
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  //Serial.println("300: WiFi MAC " + WiFi.macAddress());
  //Serial.println("310: WiFi IP  " + WiFi.localIP());
  Serial.println("300: WiFi CL initialized");
  delay(1000);
  initWebServer();
  initWebSockets();
   
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  
  initOLED();
  logo();
  initLORA();
  Serial.println("===============================");
  Serial.print("999: HASviolet available");
  Serial.println(wifi_ssid);
  Serial.println();
}


//
// MAIN
//

void loop(){
  webSocket.loop();
  // start receiver loop

  // parse for a packet, and call onReceive with the result:
  //receiveLORA(LoRa.parsePacket());

  //   test for radio signal
  //       if signal then send WS message
  //   test for WS message
  //       if WS message test for SET or TX
  //           SET then change settings
  //           TX send interrupt then msg
  //  loop receiver loop
  //
  //ws.cleanupClients();

}
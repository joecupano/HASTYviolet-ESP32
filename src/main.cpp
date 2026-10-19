//
//  HASviolet-ESP32
//
//  Supported boards: Heltec WiFi LoRa 32 V2 (SX1276) and V3 (SX1262)
//
//  Threading: the HasTRX task (core 0) is the only code that touches the radio
//  and OLED. loop() (core 1) is the only code that touches the WebSocket server.
//  They exchange data through txQueue and rxQueue.
//

//
// LIBRARIES
//

#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include "WiFi.h"
#include "HASviolet_config.h"
#include "HASviolet_boards.h"
#include "HASviolet_radio.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#ifdef HASV_OLED_YES
#include "HASviolet_OLED.h"
#endif

//
// VARIABLES
//

// Default settings before hasVIOLET.json load
String channel = "HV1";
String modemconfig = HASV_RF_LORA_MODEMCONFIG;
long frequency = HASV_RF_LORA_FREQ_HZ;
int spreadfactor = HASV_RF_LORA_SF;
int codingrate4 = HASV_RF_LORA_CR;
long bandwidth = HASV_RF_LORA_BANDWIDTH;
int txpwr = HASV_RF_LORA_TXPWR;
String mycall = "NOCALL";
String myssid = "00";
String mybeacon = "QRZ? QRZ? QRZ?";
String dstcall = "BEACON";
String dstssid = "99";

AsyncWebServer server(HASV_WEB_PORT);
WebSocketsServer webSocket(HASV_WS_PORT);

// Messages between the websocket (core 1) and the radio task (core 0)
enum MsgType : uint8_t { MSG_TX, MSG_TXPWR, MSG_TUNE, MSG_DUMP };

struct TxMsg {
  MsgType type;
  int value;                                     // MSG_TXPWR: dBm
  char text[HASV_MAX_PAYLOAD + 1];               // MSG_TX: payload, MSG_TUNE: "modemconfig:freq"
};

struct RxMsg {
  char text[HASV_MAX_PAYLOAD + 64];
};

QueueHandle_t txQueue;
QueueHandle_t rxQueue;

//
// FUNCTIONS
//

String getSubString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void queueToRadio(MsgType type, int value, const String &text) {
  TxMsg m;
  m.type = type;
  m.value = value;
  strlcpy(m.text, text.c_str(), sizeof(m.text));
  xQueueSend(txQueue, &m, 0);
}

/// Apply a ModemConfig name to the SF/CR/BW variables
void applyModemConfig() {
  if (modemconfig == "Bw500Cr45Sf128") {
    bandwidth = 500000; spreadfactor = 7; codingrate4 = 5;
  } else if (modemconfig == "Bw31_25Cr48Sf512") {
    bandwidth = 31250; spreadfactor = 7; codingrate4 = 8;
  } else if (modemconfig == "Bw125Cr48Sf4096") {
    bandwidth = 125000; spreadfactor = 12; codingrate4 = 8;
  } else if (modemconfig == "Bw125Cr45Sf2048") {
    bandwidth = 125000; spreadfactor = 8; codingrate4 = 5;
  } else {
    bandwidth = 125000; spreadfactor = 7; codingrate4 = 8;
  }
}

/// Runs in HasTRX only
void sendLORA(String outgoing) {
  if (outgoing.length() > HASV_MAX_PAYLOAD) outgoing = outgoing.substring(0, HASV_MAX_PAYLOAD);
  radioSend(outgoing);
#ifdef HASV_OLED_YES
  OLEDme(outgoing);
#endif
  Serial.print("TX:");
  Serial.println(outgoing);
}

/// WebServer
void onFavRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/favicon.ico", "image/x-icon");
}

void onIndexRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/hasVIOLET_INDEX.html", "text/html");
}

void onCssRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/hasVIOLET.css", "text/css");
}

void onJsRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/hasVIOLET.js", "text/javascript");
}

void onJsonRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/hasVIOLET.json", "application/json");
}

/// WebSockets (core 1)
void onWebSocketEvent(uint8_t clientID, WStype_t type, uint8_t *payload, size_t length) {
  if (type != WStype_TEXT) return;
  String msg = (char *)payload;

  if (msg == "SET:RADIO:RESET") {
    webSocket.sendTXT(clientID, "ACK:RADIO:RESET");
  } else if (msg == "GET:LORA") {
    queueToRadio(MSG_DUMP, 0, "");
    webSocket.sendTXT(clientID, "ACK:GET:LORA");
  } else if (msg == "SET:BEACON:ON") {
    webSocket.sendTXT(clientID, "ACK:BEACON:ON");
  } else if (msg == "SET:BEACON:OFF") {
    webSocket.sendTXT(clientID, "ACK:BEACON:OFF");
  } else if (msg == "SET:TXPWR:LOW") {
    txpwr = 7;
    queueToRadio(MSG_TXPWR, txpwr, "");
    webSocket.sendTXT(clientID, "ACK:TXPWR:LOW");
  } else if (msg == "SET:TXPWR:MEDIUM") {
    txpwr = 14;
    queueToRadio(MSG_TXPWR, txpwr, "");
    webSocket.sendTXT(clientID, "ACK:TXPWR:MEDIUM");
  } else if (msg == "SET:TXPWR:HIGH") {
    txpwr = HASV_TXPWR_MAX;
    queueToRadio(MSG_TXPWR, txpwr, "");
    webSocket.sendTXT(clientID, "ACK:TXPWR:HIGH");
  } else if (msg.startsWith("SET:TUNER:")) {
    // SET:TUNER:<modemconfig>:<frequency Hz>
    webSocket.sendTXT(clientID, "ACK:SET:TUNER:");
    queueToRadio(MSG_TUNE, 0, getSubString(msg, ':', 2) + ":" + getSubString(msg, ':', 3));
  } else if (msg.startsWith("TX:")) {
    webSocket.sendTXT(clientID, "ACK:" + msg);
    queueToRadio(MSG_TX, 0, msg.substring(3));
  }
}

/// SETUP FUNCTIONS
void initSerial() {
  Serial.begin(HASV_SERIAL_BAUD);
  Serial.println();
  Serial.println();
}

/// Power the OLED (Vext is active LOW on both boards)
void initBoard() {
  pinMode(HASV_VEXT, OUTPUT);
  digitalWrite(HASV_VEXT, LOW);
  pinMode(HASV_OLED_RST, OUTPUT);
  digitalWrite(HASV_OLED_RST, LOW);
  delay(20);
  digitalWrite(HASV_OLED_RST, HIGH);
  delay(20);
  pinMode(HASV_LED, OUTPUT);
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println(" ERR: SPIFFS mounting");
    return;
  }
  Serial.println(" 100: SPIFFs initialized");
}

void loadJsonFile() {
  File configFile = SPIFFS.open("/hasVIOLET.json", "r");
  if (!configFile) {
    Serial.println("Failed to open hasVIOLET.json for reading");
    return;
  }
  JsonDocument filter;
  filter["RADIO"] = true;
  filter["CONTACT"] = true;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile, DeserializationOption::Filter(filter));
  configFile.close();
  if (error) {
    Serial.print("Error parsing hasVIOLET.json [");
    Serial.print(error.c_str());
    Serial.println("]");
    return;
  }
  // Missing fields keep their defaults
  channel = doc["RADIO"]["channel"] | channel;
  modemconfig = doc["RADIO"]["modemconfig"] | modemconfig;
  frequency = doc["RADIO"]["frequency"] | frequency;
  spreadfactor = doc["RADIO"]["spreadfactor"] | spreadfactor;
  codingrate4 = doc["RADIO"]["codingrate4"] | codingrate4;
  bandwidth = doc["RADIO"]["bandwidth"] | bandwidth;
  txpwr = doc["RADIO"]["txpwr"] | txpwr;
  mycall = doc["CONTACT"]["mycall"] | mycall;
  myssid = doc["CONTACT"]["myssid"] | myssid;
  mybeacon = doc["CONTACT"]["mybeacon"] | mybeacon;
  dstcall = doc["CONTACT"]["dstcall"] | dstcall;
  dstssid = doc["CONTACT"]["dstssid"] | dstssid;
  Serial.println(" 200: JSON loaded");
}

void initWiFi() {
#ifdef HASV_WIFI_SSID
  WiFi.mode(WIFI_STA);
  WiFi.begin(HASV_WIFI_SSID, HASV_WIFI_KEY);
  for (int i = 0; i < HASV_WIFI_POLL_TRIES && WiFi.status() != WL_CONNECTED; i++) {
    delay(HASV_WIFI_POLL_DELAY);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" 300: WiFi client, IP ");
    Serial.println(WiFi.localIP());
    return;
  }
  Serial.println(" 300: WiFi client failed, falling back to AP");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(HASV_WIFI_APSSID, HASV_WIFI_APKEY);
  Serial.print(" 300: WiFi AP, IP ");
  Serial.println(WiFi.softAPIP());
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
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println(" 500: Websockets initialized");
}

bool initLoRa() {
  applyModemConfig();
  if (!radioBegin(frequency, bandwidth, spreadfactor, codingrate4, txpwr)) {
    return false;
  }
  Serial.println(" 250: LoRa initialized");
  return true;
}

//
// RADIO TASK (core 0)
//

void HasTRX(void *pvParameters) {
  TxMsg m;
  String incoming;
  int rssi;
  for (;;) {
    while (xQueueReceive(txQueue, &m, 0) == pdTRUE) {
      switch (m.type) {
        case MSG_TX:
          sendLORA(String(m.text));
          break;
        case MSG_TXPWR:
          radioSetTxPower(m.value);
          Serial.println("TXPWR:" + String(m.value));
          break;
        case MSG_TUNE:
          modemconfig = getSubString(String(m.text), ':', 0);
          if (getSubString(String(m.text), ':', 1).toInt() > 0) {
            frequency = getSubString(String(m.text), ':', 1).toInt();
          }
          if (!initLoRa()) Serial.println("ERR: LoRa re-init failed");
          break;
        case MSG_DUMP:
          radioDump();
          break;
      }
    }
    if (radioReceive(incoming, rssi)) {
      RxMsg r;
      snprintf(r.text, sizeof(r.text), "RX:%s|RSSI: %d", incoming.c_str(), rssi);
      Serial.println(r.text);
#ifdef HASV_OLED_YES
      OLEDme(r.text);
#endif
      xQueueSend(rxQueue, &r, 0);
    }
    vTaskDelay(1);
  }
}

//
// SETUP
//

void setup() {
  initSerial();
  Serial.println("INIT: HASviolet ESP32 (" HASV_BOARD_NAME ")");
  Serial.println("=====================================");
  initBoard();
#ifdef HASV_OLED_YES
  initOLED();
  logo();
#endif
  initSPIFFS();
  loadJsonFile();
  if (!initLoRa()) {
    Serial.println(" ERR: LoRa radio not found, halting");
#ifdef HASV_OLED_YES
    OLEDme("LoRa FAIL");
#endif
    while (true) delay(1000);
  }
  initWiFi();
  initWebServer();
  initWebSockets();

  txQueue = xQueueCreate(8, sizeof(TxMsg));
  rxQueue = xQueueCreate(8, sizeof(RxMsg));
  xTaskCreatePinnedToCore(HasTRX, "HasTRX", 8192, NULL, 1, NULL, 0);

  Serial.println("INIT: COMPLETE");
  Serial.println("=====================================");
}

//
// MAIN (core 1) - websocket only
//

void loop() {
  webSocket.loop();
  RxMsg r;
  while (xQueueReceive(rxQueue, &r, 0) == pdTRUE) {
    webSocket.broadcastTXT(r.text);
  }
  delay(1);
}

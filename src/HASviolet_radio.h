//
//  HASviolet_radio.h
//
//  Thin radio layer so main.cpp is identical for SX1276 (V2) and SX1262 (V3).
//  Call these only from the radio task (core 0) once it is running.
//

#pragma once

#if defined(HASV_LORA_SX1276)
#include <LoRa.h>
#elif defined(HASV_LORA_SX1262)
#include <RadioLib.h>
static SX1262 radio = new Module(HASV_LORA_CS, HASV_LORA_DIO1, HASV_LORA_RST, HASV_LORA_BUSY);
static volatile bool radioRxFlag = false;
static void IRAM_ATTR radioISR() { radioRxFlag = true; }
#endif

static int radioClampPower(int dbm) {
  return constrain(dbm, HASV_TXPWR_MIN, HASV_TXPWR_MAX);
}

// freq in Hz, bw in Hz, sf 6-12, cr4 5-8 (the denominator of 4/x)
bool radioBegin(long freq, long bw, int sf, int cr4, int pwr) {
  pwr = radioClampPower(pwr);
#if defined(HASV_LORA_SX1276)
  SPI.begin(HASV_LORA_SCK, HASV_LORA_MISO, HASV_LORA_MOSI, HASV_LORA_CS);
  LoRa.setPins(HASV_LORA_CS, HASV_LORA_RST, HASV_LORA_DIO0);
  if (!LoRa.begin(freq)) return false;
  LoRa.setSignalBandwidth(bw);
  LoRa.setSpreadingFactor(sf);
  LoRa.setCodingRate4(cr4);
  LoRa.enableCrc();
  LoRa.setTxPower(pwr, HASV_PABOOST ? PA_OUTPUT_PA_BOOST_PIN : PA_OUTPUT_RFO_PIN);
  return true;
#else
  SPI.begin(HASV_LORA_SCK, HASV_LORA_MISO, HASV_LORA_MOSI, HASV_LORA_CS);
  int16_t st = radio.begin(freq / 1e6, bw / 1e3, sf, cr4, RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           pwr, 8, HASV_LORA_TCXO_V);
  if (st != RADIOLIB_ERR_NONE) return false;
  radio.setDio2AsRfSwitch(true);
  radio.setCRC(2);
  radio.setPacketReceivedAction(radioISR);
  return radio.startReceive() == RADIOLIB_ERR_NONE;
#endif
}

void radioSetTxPower(int pwr) {
  pwr = radioClampPower(pwr);
#if defined(HASV_LORA_SX1276)
  LoRa.setTxPower(pwr, HASV_PABOOST ? PA_OUTPUT_PA_BOOST_PIN : PA_OUTPUT_RFO_PIN);
#else
  radio.setOutputPower(pwr);
#endif
}

void radioSend(const String &msg) {
#if defined(HASV_LORA_SX1276)
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  LoRa.receive();
#else
  String tmp = msg;
  radio.transmit(tmp);
  radio.startReceive();
#endif
}

// Returns true and fills msg/rssi when a packet has arrived
bool radioReceive(String &msg, int &rssi) {
#if defined(HASV_LORA_SX1276)
  int size = LoRa.parsePacket();
  if (size == 0) return false;
  msg = "";
  while (LoRa.available()) msg += (char)LoRa.read();
  rssi = LoRa.packetRssi();
  return true;
#else
  if (!radioRxFlag) return false;
  radioRxFlag = false;
  String data;
  int16_t st = radio.readData(data);
  radio.startReceive();
  if (st != RADIOLIB_ERR_NONE) return false;
  msg = data;
  rssi = (int)radio.getRSSI();
  return true;
#endif
}

void radioDump() {
#if defined(HASV_LORA_SX1276)
  LoRa.dumpRegisters(Serial);
#else
  Serial.println("LoRa dump: SX1262 register dump not available");
#endif
}

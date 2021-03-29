//
//  HASviolet_OLED.h
//
//
//  20210320-1700
//     
//

#include <SPI.h>
#include <Wire.h>
#include "Adafruit_BusIO_Register.h"
#include "Adafruit_GFX.h"                 // Adafruit OLED libraries - tried and true
#include "Adafruit_SSD1306.h"
#include "HVDN_logo.h"

Adafruit_SSD1306 display(HASV_OLED_WIDTH, HASV_OLED_HEIGHT, &Wire, HASV_OLED_RESET);

//
// FUNCTIONS
//


// Display myOLEDmsg on OLED
void OLEDme(String myOLEDmsg)
{
  myOLEDmsg = myOLEDmsg.substring(0,20);
  display.clearDisplay();
  //display.setTextAlignment(TEXT_ALIGN_LEFT);
  //display.setFont(ArialMT_Plain_10);
  //display.drawString(0, 15, myOLEDmsg);
  display.setCursor(0, 15);
  display.print(myOLEDmsg);
  display.display();
  //delay(oledTIME);
  //Heltec.display->clear();
}

/// Display Logo on OLED
void logo()
{
  display.clearDisplay();
  //display.drawBitmap(0,5,hvdnimg_width,hvdnimg_height,hvdnimg_bits);
  display.drawBitmap(0,5,hvdnimg_bits, hvdnimg_width,hvdnimg_height, SSD1306_WHITE);
  display.display();
  delay(1500);
  display.clearDisplay();
}

void initOLED() {
  // Start OLED with U8G2 Library
  display.begin();
  //display.setFont(ArialMT_Plain_10); 
  Serial.println(" 600: OLED Initialized");
  display.clearDisplay();
  display.display();
}

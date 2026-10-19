//
//  HASviolet_OLED.h
//
//
//  20210320-1700
//     
//

#include "Adafruit_BusIO_Register.h"
#include "Adafruit_GFX.h"                 // Adafruit OLED libraries - tried and true
#include "Adafruit_SSD1306.h"
#include "HVDN_logo.h"

Adafruit_SSD1306 display(HASV_OLED_WIDTH, HASV_OLED_HEIGHT, &Wire, HASV_OLED_RST);

static bool oledOK = false;

//
// FUNCTIONS
//


// Display myOLEDmsg on OLED
void OLEDme(String myOLEDmsg)
{
  if (!oledOK) return;
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
  if (!oledOK) return;
  display.clearDisplay();
  //display.drawBitmap(0,5,hvdnimg_width,hvdnimg_height,hvdnimg_bits);
  display.drawBitmap(0,5,hvdnimg_bits, hvdnimg_width,hvdnimg_height, SSD1306_WHITE);
  display.display();
  delay(1500);
  display.clearDisplay();
}

bool initOLED() {
  Wire.begin(HASV_OLED_SDA, HASV_OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, HASV_OLED_ADDR)) {
    Serial.println(" 600: OLED init failed");
    return false;
  }
  oledOK = true;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
  Serial.println(" 600: OLED Initialized");
  return true;
}

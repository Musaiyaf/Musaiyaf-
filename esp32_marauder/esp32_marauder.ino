/* FLASH SETTINGS
Board: LOLIN D32
Flash Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include <WiFi.h>
#include <Wire.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <Arduino.h>


#include "Assets.h"
#include "Display.h"
#include "WiFiScan.h"
#include "MenuFunctions.h"
#include "SDInterface.h"
#include "Web.h"
#include "Buffer.h"
#include "BatteryInterface.h"
#include "TemperatureInterface.h"
#include "LedInterface.h"
#include "esp_interface.h"
#include "a32u4_interface.h"
#include "settings.h"
#include "CommandLine.h"
#include "configs.h"
#include "lang_var.h"

#ifdef MARAUDER_MINI
  #include <SwitchLib.h>
  SwitchLib u_btn = SwitchLib(U_BTN, 1000, true);
  SwitchLib d_btn = SwitchLib(D_BTN, 1000, true);
  SwitchLib l_btn = SwitchLib(L_BTN, 1000, true);
  SwitchLib r_btn = SwitchLib(R_BTN, 1000, true);
  SwitchLib c_btn = SwitchLib(C_BTN, 1000, true);
#endif

Display display_obj;
WiFiScan wifi_scan_obj;
MenuFunctions menu_function_obj;
SDInterface sd_obj;
Web web_obj;
Buffer buffer_obj;
BatteryInterface battery_obj;
TemperatureInterface temp_obj;
LedInterface led_obj;
EspInterface esp_obj;
A32u4Interface a32u4_obj;
Settings settings_obj;
CommandLine cli_obj;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);

uint32_t currentTime  = 0;


void backlightOn() {
  #ifdef MARAUDER_MINI
    digitalWrite(TFT_BL, LOW);
  #endif

  #ifndef MARAUDER_MINI
    digitalWrite(TFT_BL, HIGH);
  #endif
}

void backlightOff() {
  #ifdef MARAUDER_MINI
    digitalWrite(TFT_BL, HIGH);
  #endif

  #ifndef MARAUDER_MINI
    digitalWrite(TFT_BL, LOW);
  #endif
}


void setup()
{
  pinMode(FLASH_BUTTON, INPUT);
  pinMode(TFT_BL, OUTPUT);
  backlightOff();
#if BATTERY_ANALOG_ON == 1
  pinMode(BATTERY_PIN, OUTPUT);
  pinMode(CHARGING_PIN, INPUT);
#endif
  
  // Preset SPI CS pins to avoid bus conflicts
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  Serial.begin(115200);
  
  //Serial.begin(115200);

  Serial.println("\n\nHello, World!\n");

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));
  
  display_obj.RunSetup();
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);

  backlightOff();

  // Draw the title screen
  display_obj.drawJpeg("/marauder3L.jpg", 0 , 0);     // 240 x 320 image

  //showCenterText(version_number, 250);
  #ifndef MARAUDER_MINI
    display_obj.tft.drawCentreString(display_obj.version_number, 120, 250, 2);
  #endif

  #ifdef MARAUDER_MINI
    display_obj.tft.drawCentreString(display_obj.version_number, TFT_WIDTH/2, TFT_HEIGHT, 1);
  #endif

  backlightOn(); // Need this

  delay(2000);

  display_obj.clearScreen();

  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

  display_obj.tft.println(text_table0[0]);

  delay(2000);

  display_obj.tft.println("Marauder " + display_obj.version_number + "\n");

  display_obj.tft.println(text_table0[1]);
  
  Serial.println(F("\n\n--------------------------------\n"));
  Serial.println(F("         ESP32 Marauder      \n"));
  Serial.println("            " + display_obj.version_number + "\n");
  Serial.println(F("       By: justcallmekoko\n"));
  Serial.println(F("--------------------------------\n\n"));

  //Serial.println("Internal Temp: " + (String)((temprature_sens_read() - 32) / 1.8));

  settings_obj.begin();

  Serial.println("This is a test Channel: " + (String)settings_obj.loadSetting<uint8_t>("Channel"));
  if (settings_obj.loadSetting<bool>( "Force PMKID"))
    Serial.println("This is a test Force PMKID: true");
  else
    Serial.println("This is a test Force PMKID: false");

  wifi_scan_obj.RunSetup();

  Serial.println(wifi_scan_obj.freeRAM());

  display_obj.tft.println(F(text_table0[2]));

  // Do some SD stuff
  if(sd_obj.initSD()) {
    Serial.println(F("SD Card supported"));
    display_obj.tft.println(F(text_table0[3]));
  }
  else {
    Serial.println(F("SD Card NOT Supported"));
    display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
    display_obj.tft.println(F(text_table0[4]));
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  }

  // Run display setup
  Serial.println(wifi_scan_obj.freeRAM());
  //display_obj.RunSetup();

  // Build menus
  Serial.println(wifi_scan_obj.freeRAM());
  //menu_function_obj.RunSetup();

  //display_obj.tft.println("Created Menu Structure");

  // Battery stuff
  Serial.println(wifi_scan_obj.freeRAM());
  battery_obj.RunSetup();

  display_obj.tft.println(F(text_table0[5]));

  // Temperature stuff
  Serial.println(wifi_scan_obj.freeRAM());
  temp_obj.RunSetup();

  display_obj.tft.println(F(text_table0[6]));

  battery_obj.battery_level = battery_obj.getBatteryLevel();

  if (battery_obj.i2c_supported) {
    Serial.println(F("IP5306 I2C Supported: true"));
  }
  else
    Serial.println(F("IP5306 I2C Supported: false"));

  Serial.println(wifi_scan_obj.freeRAM());

  // Do some LED stuff
  led_obj.RunSetup();

  display_obj.tft.println(F(text_table0[7]));

  delay(500);

  //display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // OG Logo Section
  /*
  digitalWrite(TFT_BL, LOW);

  // Draw the title screen
  display_obj.drawJpeg("/marauder3L.jpg", 0 , 0);     // 240 x 320 image

  //showCenterText(version_number, 250);
  display_obj.tft.drawCentreString(display_obj.version_number, 120, 250, 2);

  digitalWrite(TFT_BL, HIGH);
  */

  //esp_obj.begin();
  
  //a32u4_obj.begin(); // This goes last to make sure nothing is messed up when reading serial

  display_obj.tft.println(F(text_table0[8]));

  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);

  delay(2000);

  cli_obj.RunSetup();

  menu_function_obj.RunSetup();
}


void loop()
{
  currentTime = millis();

  // Update all of our objects
  if ((!display_obj.draw_tft) && (wifi_scan_obj.currentScanMode != ESP_UPDATE))
  {
    display_obj.main(wifi_scan_obj.currentScanMode);
    wifi_scan_obj.main(currentTime);
    sd_obj.main();
    battery_obj.main(currentTime);
    temp_obj.main(currentTime);
    settings_obj.main(currentTime);
    if ((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_EAPOL)) {
      menu_function_obj.main(currentTime);
      cli_obj.main(currentTime);
    }
      if (wifi_scan_obj.currentScanMode == OTA_UPDATE)
        web_obj.main();
    delay(1);
  }
  else if ((display_obj.draw_tft) &&
           (wifi_scan_obj.currentScanMode != OTA_UPDATE))
  {
    display_obj.drawStylus();
  }
  else if (wifi_scan_obj.currentScanMode == ESP_UPDATE) {
    display_obj.main(wifi_scan_obj.currentScanMode);
    menu_function_obj.main(currentTime);
    cli_obj.main(currentTime);
    delay(1);
  }
}

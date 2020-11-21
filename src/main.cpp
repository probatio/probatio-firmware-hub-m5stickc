#include <Arduino.h>

// Direct OSC Version: sends 44 integers for each sensor
// Needs hardcoding of IP, port, and SSID

#include <M5StickC.h>
#include "AuxFunctions.h"

#define SOFTAP 0
#define D0 1

void setup() {

  /*
      ==============
      M5 Setup
      ==============
  */

  M5.begin();
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setTextSize(5);
  dacWrite(25, 0);
  Wire.begin(0, 26);
  Serial.begin(115200);

  /*
     ==============
     Wifi Setup
     ==============
  */

  esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.print(F("Connecting to WiFi..."));
  if (!SOFTAP) {
    WiFi.begin(ssid, pass);
  }
  else {
    WiFi.softAP(ssid, pass);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP is: ");
    Serial.println(IP);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  isConnected = true;

  Serial.println();
  Serial.print(F("IP address is "));
  if (SOFTAP) Serial.println(WiFi.softAPIP());
  else Serial.println(WiFi.localIP());
}

void loop() {

  long t0 = millis();

  //M5.Lcd.fillScreen(YELLOW);

  requestI2C();
  formatBufferWithBlocks();

  //sendConsolidatedSerialMessage();
  //debugDumpBuffer();
  //sendIndividualOSCMessages();
  sendConsolidatedOSCMessage();

  // long t1 = millis();
  // long deltaT = (t1 - t0);
  // if (loopCycleControl) {
  //   if (deltaT < loopCycleTime) {
  //     delay(loopCycleTime - deltaT);
  //   }
  // }

  M5.Lcd.setCursor(20, 20);
  M5.Lcd.print(quantBlocksConnected);
}
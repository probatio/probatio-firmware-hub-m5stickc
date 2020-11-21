#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "esp_wifi.h"

WiFiUDP udp;
//
char ssid[] = "APT_1206";
char pass[] = "!jul1nh4@FRED*";

//char ssid[] = "APT_1206_EXT";
//char pass[] = "julinha1292443";

bool isConnected = false;

IPAddress destIP(192, 168, 1, 116); //
const unsigned int destPort = 7000;
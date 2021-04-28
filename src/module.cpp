// GuitarAMI Module - WiFi and file system functions
// Edu Meneses
// IDMIL - McGill University (2020)

#include <module.h>

void Module::mountFS() {

  SPIFFS.begin (true);
  
  if (SPIFFS.begin())
    printf("\nFile system mounted!\n"); 
  else
    printf("Failed to mount file system.\n\n");
}
    
    
void Module::printJSON() {
  if (SPIFFS.exists("/config.json")) { // file exists, reading and loading
    printf("Reading config file for print...\n");
    File configFile = SPIFFS.open("/config.json", "r");   // Open file for reading
      if (configFile) {
        printf("Config file:\n");
        while(configFile.available()){Serial.write(configFile.read());}
        printf("\n\n");
        }
      else {
        printf("Cannot read config file config.json. File doesn't exist.\n\n");
        }
  }
}


void Module::scanWiFi(char *deviceName, char *apPSK, char *wifiSSID, char *wifiPSK) {

  printf("\nSSID scan start\n");
  bool connectedBefore = false;
  Module::wifiScanResults.clear();

  if (WiFi.status() == WL_CONNECTED) {
    connectedBefore = true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
  }
  int n = WiFi.scanNetworks();
  printf("scan done\n");
  if (n == 0) {
      printf("no networks found\n\n");
  } else {
      printf("%d networks found\n\n",n);
      for (int i = 0; i < n; ++i) {
        wifiScanResults += i + 1;
        wifiScanResults += ": ";
        wifiScanResults += WiFi.SSID(i);
        wifiScanResults += " (";
        wifiScanResults += WiFi.RSSI(i);
        wifiScanResults += ")";
        if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
          wifiScanResults += "(no password required)";
        wifiScanResults += "<br>";
        delay(10);
      }
  }

  if (connectedBefore) {
    connectedBefore = false;
    Module::startWifi(deviceName, apPSK, wifiSSID, wifiPSK);
  }
}


void Module::startWifi(char *deviceName, char *apPSK, char *wifiSSID, char *wifiPSK) {

    WiFi.mode(WIFI_AP_STA);
    printf("The %s WiFi is set to station+softAP (setup) mode\n",deviceName);
    WiFi.softAP(deviceName, apPSK);
    printf(" Starting AP...");
    IPAddress myIP = WiFi.softAPIP();
    printf("AP IP: "); Serial.print(myIP);  // using Serial.print here (and below) out of laziness,
    if (WiFi.status() != WL_CONNECTED) {    // TODO: make IPAddresss printable
        printf(" Trying to connect to the network now...");
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(deviceName);
        WiFi.begin(wifiSSID, wifiPSK);
        long time_now = millis();
        while ( (WiFi.status() != WL_CONNECTED) && (millis() < time_now + 8000) ) {
            printf(".");
            delay(100);
        }
        if (WiFi.status() == WL_CONNECTED) {
            //delay(1000);
            printf("\nThe ");printf(deviceName);printf(" is connected to ");Serial.print(WiFi.SSID());
            printf(" network (IP address: ");Serial.print(WiFi.localIP());printf(")\n");
        }
        if (WiFi.status() == WL_DISCONNECTED)
            printf("\nFailed to connect to the network. Enter setup to connect to a different network.\n");
    }
    // Disable WiFi power save (huge latency improvements?)
      esp_wifi_set_ps(WIFI_PS_NONE);
}



// Probatio Hub - Main file
// Filipe Calegario (2020)
// Edu Meneses - IDMIL - McGill University (2021)

/*
This whole code needs some cleaning. I suggest a more C++ style,
maybe migrating to C++ Modules. 
Also, i did some workaround on Block.h and Block.cpp to deal 
with the block names. 
Also also, the AuxFunctions separation is not a great idea, unless
we modify it into a "Probatio library" (and include properly).
Edu Meneses - 2021/05/31
*/

const unsigned int firmware_version = 210531;

#include <Arduino.h>

#include <Update.h>     // For OTA over web server (manual .bin upload)
#include "mdns.h"

// Direct OSC Version: sends 44 integers for each sensor
// Needs hardcoding of IP, port, and SSID


#include "AuxFunctions.h"

// Libmapper-arduino (https://github.com/mathiasbredholt/libmapper-arduino)
#include <mapper_cpp.h>
#include <string>
#include <vector>
#include <algorithm>
mapper::Device* dev;
int lm_min = 0;
int lm_max = 256;
std::vector<int> lm_min_values;
std::vector<int> lm_max_values;
mapper::Signal lm_Signals[QUANTITY_BLOCKS]; // needs probatio_defs.h (in AuxFunctions.h)
std::vector<int> lm_Buffer;

#include <M5StickC.h> // needs to be included AFTER mapper_cpp.h

#define SOFTAP 0
#define D0 1

///////////////////////////////
// Firmware global variables //
///////////////////////////////

  struct global {
    char deviceName[25];
    char APpasswd[15];
    bool rebootFlag = false;
    unsigned long rebootTimer;
  } global;


//////////////
// settings //
//////////////

  struct Settings {
    int id = 2;
    char author[20] = {'E','d','u','_','M','e','n','e','s','e','s'};
    char institution[20] = {'I','D','M','I','L'};
    char APpasswd[15] = {'m','a','p','p','i','n','g','s'};
    char lastConnectedNetwork[30] = {'I','D','M','I','L'};
    char lastStoredPsk[30] = {'m','a','p','p','i','n','g','s'};
    int32_t firmware = 0;
    char oscIP[17] = {'0','0','0','0'};
    int32_t oscPORT = 8000;
    int32_t localPORT = 8000;
    } settings;


////////////////////////////////////////////////////////////
// Include GuitarAMI WiFi and json module functions files //
////////////////////////////////////////////////////////////

  #include "module.h"

  DNSServer dnsServer;
  AsyncWebServer server(80);
  Module module;

//////////////////////////
// Forward declarations //
//////////////////////////

  void printVariables();
  void printVariablesRaw();
  void parseJSON();
  void saveJSON();
  void initWebServer();
  void start_mdns_service();
  String indexProcessor(const String& var);
  String scanProcessor(const String& var);
  String factoryProcessor(const String& var);
  String updateProcessor(const String& var);


void setup() {
    // put your setup code here, to run once:
        Serial.begin(115200);
    
    // M5 Setup
        M5.begin();
        M5.Lcd.fillScreen(BLUE);
        M5.Lcd.setTextSize(5);
        dacWrite(25, 0);
        Wire.begin(0, 26);
        Serial.begin(115200);

    // Start FS and check Json file (config.json)
        module.mountFS();
        module.printJSON();

    // Load Json file stored values
        parseJSON();
        settings.firmware = firmware_version;
        saveJSON();

    printf( "\n"
        "Probatio\n"
        "module ID: %03i\n"
        "Version %d\n"
        "\n"
        "Booting System...\n",settings.id,settings.firmware);

    // Print Json file stored values
        //printJSON();
        printVariables();
        //printVariablesRaw();

    // Define this module full name
        snprintf(global.deviceName,(sizeof(global.deviceName)-1),"probatio_m5_%03i",settings.id);

    // Connect to WiFi, web server, and start dns server
        module.scanWiFi(global.deviceName, settings.APpasswd, settings.lastConnectedNetwork, settings.lastStoredPsk);
        module.startWifi(global.deviceName, settings.APpasswd, settings.lastConnectedNetwork, settings.lastStoredPsk);
        Serial.println("Starting DNS server");
        dnsServer.start(53, "*", WiFi.softAPIP());
        start_mdns_service();
        initWebServer();

    // Create device for libmapper
        if (WiFi.status() == WL_CONNECTED) {
            std::string lm_name = global.deviceName;
            dev = new mapper::Device(lm_name);
        }

    Serial.println("\n\nBoot complete\n\n");
}

void loop() {

    dnsServer.processNextRequest();

    //long t0 = millis();

    //M5.Lcd.fillScreen(YELLOW);

    requestI2C();
    formatBufferWithBlocks();

    //sendConsolidatedSerialMessage();
    //debugDumpBuffer();
    //sendIndividualOSCMessages(global.deviceName, settings.oscIP, settings.oscPORT);
    sendConsolidatedOSCMessage(global.deviceName, settings.oscIP, settings.oscPORT);

    // Do libmapper stuff
    dev->poll();

    // Create signal for libmapper if block is connected and update signals
    for (int i = 0; i < QUANTITY_BLOCKS; i++) {
        if (blocks[i]->isConnected) {
            if (!blocks[i]->libmapper) {
                lm_min_values.clear();
                lm_max_values.clear();
                for (int k = 0; k < blocks[i]->quantity; k++) {
                    lm_min_values.push_back(lm_min);
                    lm_max_values.push_back(lm_max);
                }
                lm_Signals[i] = dev->add_signal(mapper::Direction::OUTGOING, blocks[i]->string_name.c_str(), 
                                blocks[i]->quantity,mapper::Type::INT32, 0, &lm_min_values, &lm_max_values);
                blocks[i]->libmapper = true;
            } 
            else {
                lm_Buffer.clear();
                for (int j = 0; j < blocks[i]->quantity; j++) {
                    lm_Buffer.push_back((int)blocks[i]->values[j].getValue());
                }
                lm_Signals[i].set_value(lm_Buffer);
            }
        } else if (blocks[i]->libmapper) {
            dev->remove_signal(lm_Signals[i]);
            blocks[i]->libmapper = false;
        }
    }

    // long t1 = millis();
    // long deltaT = (t1 - t0);
    // if (loopCycleControl) {
    //   if (deltaT < loopCycleTime) {
    //     delay(loopCycleTime - deltaT);
    //   }
    // }

    // Checking for timed reboot (called by setup mode) - reboots after 2 seconds
        if (global.rebootFlag && (millis() - 3000 > global.rebootTimer)) {
            ESP.restart();
        }

    M5.Lcd.setCursor(20, 20);
    M5.Lcd.print(quantBlocksConnected);
}

///////////////
// Functions //
///////////////

void printVariables() {
    Serial.println("Printing loaded values (settings):");
    Serial.print("ID: "); Serial.println(settings.id);
    Serial.print("Designer: "); Serial.println(settings.author);
    Serial.print("Institution: "); Serial.println(settings.institution);
    Serial.print("AP password: "); Serial.println(settings.APpasswd);
    Serial.print("Saved SSID: "); Serial.println(settings.lastConnectedNetwork);
    Serial.print("Saved SSID password: "); Serial.println("********");
    Serial.print("Firmware version: "); Serial.println(settings.firmware);
    Serial.print("OSC IP: "); Serial.println(settings.oscIP);
    Serial.print("OSC port: "); Serial.println(settings.oscPORT);
    Serial.print("Local port: "); Serial.println(settings.localPORT);
    Serial.println();
}

void printVariablesRaw() {
    Serial.println("Printing loaded values (settings):");
    Serial.print("settings.id: "); Serial.println(settings.id);
    Serial.print("settings.author: "); Serial.println(settings.author);
    Serial.print("settings.institution: "); Serial.println(settings.institution);
    Serial.print("settings.APpasswd: "); Serial.println(settings.APpasswd);
    Serial.print("settings.lastConnectedNetwork: "); Serial.println(settings.lastConnectedNetwork);
    Serial.print("settings.lastStoredPsk: "); Serial.println(settings.lastStoredPsk);
    Serial.print("settings.firmware: "); Serial.println(settings.firmware);
    Serial.print("settings.oscIP: "); Serial.println(settings.oscIP);
    Serial.print("settings.oscPORT: "); Serial.println(settings.oscPORT);
    Serial.print("settings.localPORT: "); Serial.println(settings.localPORT);
    Serial.println();
}

void parseJSON() {    
  /* 
  Allocate a temporary JsonDocument
    Don't forget to change the capacity to match your requirements if using DynamicJsonDocument
    Use https://arduinojson.org/v6/assistant/ to compute the capacity.
    const size_t capacity = JSON_OBJECT_SIZE(15) + 250;
    DynamicJsonDocument doc(capacity);
  */
    StaticJsonDocument<192> doc;

    if (SPIFFS.exists("/config.json")) { // file exists, reading and loading
        
        Serial.println("Reading config file...");
        
        File configFile = SPIFFS.open("/config.json", "r");
        
        // Deserialize the JSON document
        StaticJsonDocument<384> doc;
        DeserializationError error = deserializeJson(doc, configFile);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        // Copy values from the JsonDocument to the Config
        settings.id = doc["id"];
        strlcpy(settings.author, doc["author"], sizeof(settings.author));
        strlcpy(settings.institution, doc["institution"], sizeof(settings.institution));
        strlcpy(settings.APpasswd, doc["APpasswd"], sizeof(settings.APpasswd));
        strlcpy(settings.lastConnectedNetwork, doc["lastConnectedNetwork"], sizeof(settings.lastConnectedNetwork));
        strlcpy(settings.lastStoredPsk, doc["lastStoredPsk"], sizeof(settings.lastStoredPsk));
        settings.firmware = doc["firmware"];
        strlcpy(settings.oscIP, doc["oscIP"], sizeof(settings.oscIP));
        settings.oscPORT = doc["oscPORT"];
        settings.localPORT = doc["localPORT"];

        configFile.close();
        
        Serial.println("Probatio configuration file loaded.\n");
    } 
    else
        Serial.println("Failed to read config file!\n");
    }

void saveJSON() { // Serializing
  
    Serial.println("Saving config to JSON file...");

    /*
    Allocate a temporary JsonDocument
        Don't forget to change the capacity to match your requirements if using DynamicJsonDocument
        Use https://arduinojson.org/v6/assistant/ to compute the capacity.
        
        const size_t capacity = JSON_OBJECT_SIZE(15);
        DynamicJsonDocument doc(capacity);
    */

    StaticJsonDocument<256> doc;

    // Copy values from Config to the JsonDocument
        doc["id"] = settings.id;
        doc["author"] = settings.author;
        doc["institution"] = settings.institution;
        doc["APpasswd"] = settings.APpasswd;
        doc["lastConnectedNetwork"] = settings.lastConnectedNetwork;
        doc["lastStoredPsk"] = settings.lastStoredPsk;
        doc["firmware"] = settings.firmware;
        doc["oscIP"] = settings.oscIP;
        doc["oscPORT"] = settings.oscPORT;
        doc["localPORT"] = settings.localPORT;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {Serial.println("Failed to open config file for writing!\n");}
    
    // Serialize JSON to file
        if (serializeJson(doc, configFile) == 0)
        Serial.println("Failed to write to file");
        else
        Serial.println("JSON file successfully saved!\n");
    
    configFile.close();
} //end save

//////////
// Site //
//////////

String indexProcessor(const String& var) {
    if(var == "DEVICENAME")
        return global.deviceName;
    if(var == "STATUS") {
        if (WiFi.status() != WL_CONNECTED) {
        char str[40]; 
        snprintf(str, sizeof(str), "Currently not connected to any network");
        return str;
        } else {
        char str[120];
        snprintf (str, sizeof(str), "Currently connected on <strong style=\"color:Tomato;\">%s</strong> network (IP: %s)", settings.lastConnectedNetwork, WiFi.localIP().toString().c_str());
        return str;
        }
    }
    if(var == "CURRENTSSID")
        return WiFi.SSID();
    if(var == "CURRENTPSK")
        return settings.lastStoredPsk;
    if(var == "CURRENTIP")
        return WiFi.localIP().toString();
    if(var == "CURRENTAPIP")
        return WiFi.softAPIP().toString();
    if(var == "CURRENTSTAMAC")
        return WiFi.macAddress();
    if(var == "CURRENTAPMAC")
        return WiFi.softAPmacAddress();
    if(var == "CURRENTOSC")
        return settings.oscIP;
    if(var == "CURRENTPORT") {
        char str[7]; 
        snprintf(str, sizeof(str), "%d", settings.oscPORT);
        return str;
    }
    if(var == "PROBATIOID") {
        char str[4];
        snprintf (str, sizeof(str), "%03d", settings.id);
        return str;
    }
    if(var == "PROBATIOAUTH")
        return settings.author;
    if(var == "PROBATIOINST")
        return settings.institution;
    if(var == "PROBATIOVER")
        return String(settings.firmware);
        
    return String();
}

String factoryProcessor(const String& var) {
    if(var == "DEVICENAME")
        return global.deviceName;
    if(var == "STATUS") {
        if (WiFi.status() != WL_CONNECTED) {
        char str[40]; 
        snprintf(str, sizeof(str), "Currently not connected to any network");
        return str;
        } else {
        char str[120];
        snprintf (str, sizeof(str), "Currently connected on <strong style=\"color:Tomato;\">%s</strong> network (IP: %s)", settings.lastConnectedNetwork, WiFi.localIP().toString().c_str());
        return str;
        }
    }
    if(var == "PROBATIOID") {
        char str[4];
        snprintf (str, sizeof(str), "%03d", settings.id);
        return str;
    }
    if(var == "PROBATIOVER")
        return String(settings.firmware);
    return String();
}

String scanProcessor(const String& var) {
    if(var == "SSIDS")
        return module.wifiScanResults;
        
    return String();
}

String updateProcessor(const String& var) {
    if(var == "UPDATESTATUSF")
        return module.wifiScanResults;
    if(var == "UPDATESTATUSS")
        return module.wifiScanResults;
        
    return String();
}

void initWebServer() {

    // Route for root page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", String(), false, indexProcessor);
    });
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      printf("\nSettings received! (HTTP_post):\n");
      if(request->hasParam("reboot", true)) {
          request->send(SPIFFS, "/reboot.html");
          global.rebootFlag = true;
          global.rebootTimer = millis();
      } else {
        if(request->hasParam("SSID", true)) {
            //Serial.println("SSID received");
            strcpy(settings.lastConnectedNetwork, request->getParam("SSID", true)->value().c_str());
            printf("SSID stored: %s\n", settings.lastConnectedNetwork);
        } else {
            printf("No SSID received\n");
        }
        if(request->hasParam("password", true)) {
            //Serial.println("SSID Password received");
            strcpy(settings.lastStoredPsk, request->getParam("password", true)->value().c_str());
            printf("SSID password stored: ********\n");
        } else {
            printf("No SSID password received\n");
        }
        if(request->hasParam("APpasswd", true) && request->hasParam("APpasswdValidate", true)) {
            if(request->getParam("APpasswd", true)->value() == request->getParam("APpasswdValidate", true)->value() && request->getParam("APpasswd", true)->value() != "") {
              strcpy(settings.APpasswd, request->getParam("APpasswd", true)->value().c_str());
              printf("AP password stored: %s\n", settings.APpasswd);
            } else {
              printf("AP password blank or not match retype. Keeping old password\n");
            }
        } else {
            printf("No AP password received\n");
        }
        if(request->hasParam("oscIP", true)) {
            if(request->getParam("oscIP", true)->value() == ""){
              strcpy(settings.oscIP,"0.0.0.0");
            } else {
              strcpy(settings.oscIP, request->getParam("oscIP", true)->value().c_str());
            }
            printf("OSC IP received: %s\n", settings.oscIP);
        } else {
            printf("No OSC IP received\n");
        }
        if(request->hasParam("oscPORT", true)) {
            settings.oscPORT = atoi(request->getParam("oscPORT", true)->value().c_str());
            printf("OSC port received: %d\n", settings.oscPORT);
        } else {
            printf("No OSC port received\n");
        }
        request->send(SPIFFS, "/index.html", String(), false, indexProcessor);
        //request->send(200);
      }
      saveJSON();
    });

  // Route for scan page
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/scan.html", String(), false, scanProcessor);
    });

    // Route for instructions page
    server.on("/instructions", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/instructions.html");
    });

  // Route for factory page
    server.on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/factory.html", String(), false, factoryProcessor);
    });
    server.on("/factory", HTTP_POST, [](AsyncWebServerRequest *request) {
        printf("\nFactory Settings received! (HTTP_post):\n");
        if(request->hasParam("reboot", true)) {
            request->send(SPIFFS, "/reboot.html");
            global.rebootFlag = true;
            global.rebootTimer = millis();
        } else {
          if(request->hasParam("ID", true)) {
              settings.id = atoi(request->getParam("ID", true)->value().c_str());
              printf("ID (factory) received: %d\n", settings.id);
          } else {
              printf("No ID (factory) received\n");
          }
          if(request->hasParam("firmware", true)) {
              settings.firmware = atoi(request->getParam("firmware", true)->value().c_str());
              printf("Firmware # (factory) received: %d\n", settings.firmware);
          } else {
              printf("No Firmware # (factory) received\n");
          }
          request->send(SPIFFS, "/factory.html", String(), false, factoryProcessor);
          //request->send(200);
        }
        saveJSON();
      });

  // Route for update page
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/update.html");
    });
    /*handling uploading firmware file */
    server.on("/updateF", HTTP_POST, [](AsyncWebServerRequest *request){
      global.rebootFlag = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", global.rebootFlag?"Update complete, rebooting":"Fail to send file and/or update");
      response->addHeader("Connection", "close");
      request->send(response);
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        Serial.printf("Firmware Update Start: %s\n", filename.c_str());
        //Update.runAsync(true);
        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
          Update.printError(Serial);
        }
      }
      if(!Update.hasError()){
        if(Update.write(data, len) != len){
          Update.printError(Serial);
        }
      }
      if(final){
        if(Update.end(true)){
          Serial.printf("Firmware Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
      }
    });     
    /*handling uploading SPIFFS file */
    server.on("/updateS", HTTP_POST, [](AsyncWebServerRequest *request){
      global.rebootFlag = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", global.rebootFlag?"Update complete, rebooting":"Fail to send file and/or update");
      response->addHeader("Connection", "close");
      request->send(response);
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        Serial.printf("SPIFFS Update Start: %s\n", filename.c_str());
        //Update.runAsync(true);
        if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)){
          Update.printError(Serial);
        }
      }
      if(!Update.hasError()){
        if(Update.write(data, len) != len){
          Update.printError(Serial);
        }
      }
      if(final){
        if(Update.end(true)){
          Serial.printf("SPIFFS Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
      }
    });  

  // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/style.css", "text/css");
    });

  server.begin();
  
  Serial.println("HTTP server started");
}

void start_mdns_service() {
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set(global.deviceName);
    //set default instance
    mdns_instance_name_set(global.deviceName);
}

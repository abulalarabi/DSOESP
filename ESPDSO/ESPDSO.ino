#include <WiFiManager.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define LINE_BUFFER_SIZE 128

AsyncWebServer server(80);
int nextFileIndex = 0;


void setup() {
  Serial.setRxBufferSize(4096); // 4KB buffer instead of default 256 bytes
  Serial.begin(115200);
  Serial.setTimeout(50); // Shorter timeout for faster processing
  
  WiFi.mode(WIFI_STA);
  LittleFS.begin(true);
  findLastFileIndex();

  WiFiManager wm;
  if (!wm.autoConnect("DSOESP", "DSOESPARABI")) {
    ESP.restart();
  }

  ArduinoOTA.begin();
  initWebServer();
}

void loop() {
  // Handle OTA but give priority to serial data
  if (!Serial.available()) {
    ArduinoOTA.handle();
  }
  
  static String currentData = "";
  static unsigned long lastByteTime = 0;
  static bool hasData = false;
  const unsigned long TIMEOUT_MS = 800; // Shorter timeout
  
  // Priority: Read all available serial data immediately
  while (Serial.available()) {
    currentData += (char)Serial.read();
    lastByteTime = millis();
    hasData = true;
    
    // Prevent memory overflow
    if (currentData.length() > 30000) {
      // Force save if data gets too large
      String filename = "/data_" + String(nextFileIndex++) + ".csv";
      File file = LittleFS.open(filename, FILE_WRITE);
      if (file) {
        file.print(currentData);
        file.close();
      }
      currentData = "";
      hasData = false;
      return; // Return immediately to continue reading
    }
  }
  
  // Save when data stream ends
  if (hasData && (millis() - lastByteTime > TIMEOUT_MS)) {
    if (currentData.length() > 100) { // Only save substantial data
      String filename = "/data_" + String(nextFileIndex++) + ".csv";
      File file = LittleFS.open(filename, FILE_WRITE);
      if (file) {
        file.print(currentData);
        file.close();
      }
    }
    currentData = "";
    hasData = false;
  }
}
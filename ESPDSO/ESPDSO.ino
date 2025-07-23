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
  Serial.begin(115200);
  Serial.setTimeout(0);
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
  ArduinoOTA.handle();

  if (Serial.available() > 0) {
    // Create a new file
    String filename = "/data_" + String(nextFileIndex++) + ".csv";
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) return;

    // Buffer to store each line
    char lineBuffer[LINE_BUFFER_SIZE];
    size_t index = 0;

    // Read and write line by line
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || index >= LINE_BUFFER_SIZE - 1) {
        lineBuffer[index] = '\0';
        file.println(lineBuffer);
        index = 0;
      } else {
        lineBuffer[index++] = c;
      }
      delay(1);  // small yield to keep system responsive
    }

    file.close();
  }
}

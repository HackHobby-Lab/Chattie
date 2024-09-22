
#include <WiFi.h>
#include "localServer.h"


void setup() {
  
    Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  

  startAPMode();
}

void loop() {
  server.handleClient();
}


#include "global.h"
#include "taskConnectWifi.h"
#include "taskLedBlink.h"
#include "taskWebServer.h"
#include "coreiot.h"

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    xTaskCreate(taskLedBlink, "LED Blink", 4096, nullptr, 1, nullptr);
    xTaskCreate(taskConnectWifi, "Connect WiFi", 4096, NULL, 1, NULL);
    delay(500);
    xTaskCreate(taskWebServer, "Web Server", 8192, NULL, 1, NULL);
    delay(500);
    xTaskCreate(coreiot_task, "CoreIoT", 6144, NULL, 1, NULL);
}

void loop() {}

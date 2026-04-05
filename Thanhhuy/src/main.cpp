#include "global.h"
#include "taskConnectWifi.h"
#include "taskLedBlink.h"
#include "taskTempHumi.h"
#include "taskWebServer.h"

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    xTaskCreate(taskLedBlink, "LED Blink", 4096, nullptr, 1, nullptr);
    xTaskCreate(taskTempHumi, "Temp and Humi", 2000, NULL, 1, NULL);
    xTaskCreate(taskConnectWifi, "Connect WiFi", 4096, NULL, 1, NULL);
    delay(500);
    xTaskCreate(taskWebServer, "Web Server", 8192, NULL, 1, NULL);
    delay(500);
}

void loop() {}

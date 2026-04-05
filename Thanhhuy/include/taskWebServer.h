#ifndef __TASK_WEB_SERVER__
#define __TASK_WEB_SERVER__
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "global.h"

// Hàm test WiFi connection
String testWiFiConnection(String ssid, String password);

void taskWebServer(void* pvParameters);

#endif
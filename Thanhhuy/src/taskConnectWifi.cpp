#include "taskConnectWifi.h"

void taskConnectWifi(void* pvParameters) {
    Serial.println("Task Connect WiFi: Starting...");
    Serial.println("Waiting for WiFi configuration from web server...");

    // Đợi cho đến khi có thông tin WiFi từ web form
    while (!wifi_configured) {
        Serial.println("Waiting for WiFi credentials...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    Serial.println("WiFi credentials received! Starting connection...");
    Serial.print("SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("Password: ");
    Serial.println(wifi_password);

    WiFi.disconnect();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    Serial.print("Connecting to WiFi: ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    int attempts = 0;
    const int maxAttempts = 60;

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        attempts++;
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed!");
        Serial.println("Please check SSID and password.");
    }
    while (1) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected! Reconnecting...");
            WiFi.disconnect();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

            attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
                attempts++;
            }

            Serial.println();
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFi reconnected!");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
            } else {
                Serial.println("Reconnection failed!");
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
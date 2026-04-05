#include "taskLedBlink.h"
#include "taskTempHumi.h"
#include "global.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <Adafruit_NeoPixel.h>

static const float TEMP_LOW = 25.0f;
static const float TEMP_HIGH = 30.0f;

static uint32_t g_onMs = 1000;
static uint32_t g_offMs = 1000;

static uint8_t g_ledPin = 45;
static const uint16_t g_ledCount = 1;
static Adafruit_NeoPixel g_pixels(g_ledCount, g_ledPin, NEO_GRB + NEO_KHZ800);

static uint32_t g_ledColor = 0;

static void updateBlinkRates(float tempC, float humi) {
    (void)humi;

    if (tempC > TEMP_HIGH) {
        g_onMs = 200;
        g_offMs = 200;
        g_ledColor = g_pixels.Color(255, 0, 0);
        return;
    }

    if (tempC >= TEMP_LOW) {
        g_onMs = 1000;
        g_offMs = 1000;
        g_ledColor = g_pixels.Color(0, 255, 0);
        return;
    }

    g_onMs = 2000;
    g_offMs = 2000;
    g_ledColor = g_pixels.Color(255, 255, 0);
}

static void ledTask(void *pvParameters) {
    (void)pvParameters;

    uint32_t localOnMs = 1000;
    uint32_t localOffMs = 1000;
    float localTemp = 0.0f;
    float localHumi = 0.0f;

    while (true) {
        if (xSemaphoreTake(g_tempHumiSemaphore, 0) == pdTRUE) {
            if (xSemaphoreTake(g_tempHumiMutex, portMAX_DELAY) == pdTRUE) {
                localTemp = glob_temperature;
                localHumi = glob_humidity;
                updateBlinkRates(localTemp, localHumi);
                localOnMs = g_onMs;
                localOffMs = g_offMs;
                xSemaphoreGive(g_tempHumiMutex);

                Serial.print("Temp/Humi updated: ");
                Serial.print(localTemp, 1);
                Serial.print(" C, ");
                Serial.print(localHumi, 1);
                Serial.println(" %");
            }
        }

        if (glob_led_enabled) {
            g_pixels.setPixelColor(0, g_ledColor);
            g_pixels.show();
            vTaskDelay(pdMS_TO_TICKS(localOnMs));
            g_pixels.clear();
            g_pixels.show();
            vTaskDelay(pdMS_TO_TICKS(localOffMs));
        } else {
            g_pixels.clear();
            g_pixels.show();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void taskLedBlink(void *pvParameters) {
    (void)pvParameters;

    g_pixels.begin();
    g_pixels.clear();
    g_pixels.show();

    g_tempHumiSemaphore = xSemaphoreCreateBinary();
    g_tempHumiMutex = xSemaphoreCreateMutex();

    if (g_tempHumiSemaphore == nullptr || g_tempHumiMutex == nullptr) {
        Serial.println("Failed to create temp/humi sync objects");
        vTaskDelete(nullptr);
    }

    xTaskCreate(taskTempHumi, "TempHumiTask", 3072, nullptr, 2, nullptr);
    xTaskCreate(ledTask, "LedTask", 2048, nullptr, 1, nullptr);

    vTaskDelete(nullptr);
}

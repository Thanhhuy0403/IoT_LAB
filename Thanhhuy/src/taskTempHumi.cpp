#include "taskTempHumi.h"

float glob_temperature = 1;
float glob_humidity = 1;
SemaphoreHandle_t g_tempHumiSemaphore = nullptr;
SemaphoreHandle_t g_tempHumiMutex = nullptr;

DHT20 DHT(&Wire);

void taskTempHumi(void* pvParameters) {
    Wire.begin(11, 12);
    delay(2000);
    while (1) {
        if (DHT.read() == 0) {
            if (xSemaphoreTake(g_tempHumiMutex, portMAX_DELAY) == pdTRUE) {
                glob_humidity = DHT.getHumidity();
                glob_temperature = DHT.getTemperature();
                xSemaphoreGive(g_tempHumiMutex);
            }
            xSemaphoreGive(g_tempHumiSemaphore);
        } else {
            Serial.println("DHT20 read failed");
        }
        Serial.print("Humi: ");
        Serial.print(glob_humidity, 1);
        Serial.print(" %,\tTemp: ");
        Serial.print(glob_temperature, 1);
        Serial.println(" C");
        vTaskDelay(5000);
    }
}
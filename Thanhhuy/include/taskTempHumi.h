#ifndef __TASK_TEMP_HUMI__
#define __TASK_TEMP_HUMI__
#include <Arduino.h>

#include "DHT20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;
extern SemaphoreHandle_t g_tempHumiSemaphore;
extern SemaphoreHandle_t g_tempHumiMutex;

void taskTempHumi(void* pvParameters);

#endif
#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>
#define LED_GPIO 48         // Default led on board

void TaskLEDControl(void *pvParameters);

#endif  
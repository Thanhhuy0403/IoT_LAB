#pragma once

#include <Arduino.h>

// RTOS task that sets up semaphores and manages LED blinking with temperature.
void taskLedBlink(void *pvParameters);

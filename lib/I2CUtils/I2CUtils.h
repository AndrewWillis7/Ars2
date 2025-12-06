#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace I2CUtils {

extern SemaphoreHandle_t I2C_Mutex;

// I2C Mux Address
static const uint8_t MUX_ADDR = 0x70;

// setup
void begin();

// Mutex helpers
inline void i2cLock() {
    xSemaphoreTake(I2C_Mutex, portMAX_DELAY);
}

inline void i2cUnlock() {
    xSemaphoreGive(I2C_Mutex);
}

// Select mux channel safely
bool selectChannel(uint8_t ch) {}

} // eol namespace


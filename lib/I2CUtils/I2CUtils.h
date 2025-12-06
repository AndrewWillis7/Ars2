#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <hw_config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace I2CUtils {

extern SemaphoreHandle_t I2C_Mutex;

// I2C Mux Address
static const uint8_t SDA_PIN = HW_I2C_SDA;
static const uint8_t SCL_PIN = HW_I2C_SCL;
static const uint8_t MUX_ADDR = HW_I2C_MUX_ADDR;

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
bool selectChannel(uint8_t ch);

bool busIsStuck();
bool recoverBus();

} // eol namespace


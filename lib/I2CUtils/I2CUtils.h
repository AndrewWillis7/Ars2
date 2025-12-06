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

// Stats
extern uint32_t totalLocks;
extern uint32_t totalUnlocks;
extern uint32_t totalMutexWaits;

// setup
void begin();

// Mutex helpers
inline void i2cLock() {
    uint32_t t0 = micros();
    xSemaphoreTake(I2C_Mutex, portMAX_DELAY);
    totalLocks++;
    totalMutexWaits += (micros() - t0);
}

inline void i2cUnlock() {
    xSemaphoreGive(I2C_Mutex);
    totalUnlocks++;
}

// Select mux channel safely
bool selectChannel(uint8_t ch);

// AutoRepair
bool busIsStuck();
bool recoverBus();
bool guardAndRecover();

void printI2CStats();

} // eol namespace


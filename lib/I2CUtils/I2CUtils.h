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

// Track the channel currently active on the bus
// 0xFF is bad
extern volatile uint8_t currentChannel;

// -- setup --
void begin();

// -- Mutex helpers --
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

inline uint8_t getCurrentChannel() {
    return currentChannel;
}

// -- Mux Helper methods --

// Select mux channel safely
// Ideally, return true and program the mux on success, return false on any error
bool selectChannel(uint8_t ch);

// ensure the mux is on the proper channel, if not call selectChannel()
bool ensureChannel(uint8_t ch);

// RAII guard (Resource Acquisition Is Initialization) for:
//  * Lock the bus
//  * Select this channel
//  * unlock when scope exits

class ScopedI2C {
public:
    explicit ScopedI2C(uint8_t muxChannel)
        : _ok(false)
    {
        i2cLock();
        _ok = selectChannel(muxChannel);
    }

    ~ScopedI2C() {
        i2cUnlock();
    }

    bool ok() const {return _ok;}

private:
    bool _ok;
};

class ScopedLock {
public:
    ScopedLock() {i2cLock();}
    ~ScopedLock() {i2cUnlock();}
};

// AutoRepair
bool busIsStuck();
bool recoverBus();
bool guardAndRecover();

void printI2CStats();

} // eol namespace


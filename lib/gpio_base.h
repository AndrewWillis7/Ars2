#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class GPIOBase {
public:
    GPIOBase(const char* name)
        : _name(name), _taskHandle(nullptr), _intervalMs(20)
    {}

    virtual ~GPIOBase() {}

    virtual void setup() = 0;
    virtual void readRaw() = 0;

    // Start on Core 1 by default for GPIO sensors
    void startTask(uint32_t intervalMs = 20, BaseType_t core = 1, UBaseType_t priority = 1) {
        _intervalMs = intervalMs;
        xTaskCreatePinnedToCore(
            _taskEntry,
            _name,
            4096,
            this,
            priority,
            &_taskHandle,
            core
        );
    }

    void stopTask() {
        if (_taskHandle) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
    }

    bool isAlive() const {
        return (millis() - _lastHeartbeat) < 2000; // If sensor doesn't output data for 2 seconds, sensor is dead yall
    }

    virtual void debugPrint() {
        Serial.printf("[%s] (GPIO sensor has no debug override)\n", _name);
    }

    void printStats() {
        Serial.printf("[%s] reads=%u last=%u ms\n",
            _name,
            _readCount,
            millis() - _lastHeartbeat
        );
    }

protected:
    const char* _name;
    TaskHandle_t _taskHandle;
    uint32_t _intervalMs;
    uint32_t _lastHeartbeat = 0;
    uint32_t _readCount = 0;

private:
    static void _taskEntry(void* ptr) {
        reinterpret_cast<GPIOBase*>(ptr)->taskLoop();
    }

    void taskLoop() {
        for (;;) {
            uint32_t start = micros();

            readRaw();

            _readCount++;
            _lastHeartbeat = millis();

            vTaskDelay(_intervalMs / portTICK_PERIOD_MS);
        }
    }
};
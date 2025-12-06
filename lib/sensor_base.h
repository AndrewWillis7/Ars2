#pragma once
#include <Arduino.h>
#include <I2CUtils.h>
#include "scheduler.h"

class SensorBase {
public:
    friend class Scheduler;
    SensorBase(const char* name, uint8_t muxChannel) :
        _name(name), 
        _muxChannel(muxChannel),
        _taskHandle(nullptr), 
        _taskIntervalMs(50) 
    {
        Scheduler::instance().registerSensor(this);
    }
    
    virtual ~SensorBase() {}

    // -- Setup --
    virtual void setup() = 0;

    // -- Blocking Aqui --
    virtual void readRaw() = 0;
    virtual void readBlocking() {
        I2CUtils::ScopedI2C guard(_muxChannel);
        if (!guard.ok()) {
            // Mux Failure
            return;
        }
        readRaw();
        _lastReadTime = millis();
    };

    // Called to force a synchronous update
    void updateBlocking() {readBlocking();}

    // -- Task-based Aqui (Optional technically) --
    void startTask(uint32_t intervalMs = 20, BaseType_t core = tskNO_AFFINITY) {
        _taskIntervalMs = intervalMs;
        xTaskCreatePinnedToCore(
            _taskEntry, _name, 4096, this, 1, &_taskHandle, core
        );
    }

    bool taskRunning() const {return _taskHandle != nullptr;}

    // task Control
    void stopTask() {
        if (_taskHandle) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
    }

    void pauseTask() {_paused = true;}
    void resumeTask() {_paused = false;}
    bool isPaused() const {return _paused;}

    // -- Debug --
    virtual void debugPrint() {
        Serial.printf("[%s] No debugPrint override.\n", _name);
    }

    bool isAlive() const {
        return (millis() - _lastHeartbeat < 2000); // 2 sec no signal? Dead.
    }

    void printStats() {
        Serial.printf(
            "[%s] core=%u reads=%u last=%uus avg=%uus mutexWait=%uus hb=%ums\n",
            _name,
            _taskCore,
            _readCount,
            _lastReadDuration,
            _avgReadDuration,
            _mutexWaitTime,
            millis() - _lastHeartbeat
        );
    }

    
protected:
    const char* _name;
    uint8_t _muxChannel;
    TaskHandle_t _taskHandle;
    uint32_t _taskIntervalMs;
    uint32_t _lastReadTime = 0;
    uint32_t _lastReadDuration = 0;
    uint32_t _avgReadDuration = 0;
    uint32_t _readCount = 0;
    uint32_t _mutexWaitTime = 0;
    uint32_t _lastHeartbeat = 0;
    uint32_t _taskCore = 0;
    volatile bool _paused = false;

    uint32_t _minInterval = 10;
    uint32_t _maxInterval = 200;
    uint32_t _currentInterval = 50;

private:
    // static call for FreeRTOS
    static void _taskEntry(void* ptr) {
        reinterpret_cast<SensorBase*>(ptr)->taskLoop();
    }

    void taskLoop() {
        _taskCore = xPortGetCoreID();

        for (;;) {
            if (_paused) {
                vTaskDelay(5 / portTICK_PERIOD_MS);
                continue;
            }

            uint32_t readStart = micros();

            {
                // FULL LOCK during entire sensor transaction
                I2CUtils::ScopedLock lock;

                // Select mux channel
                if (!I2CUtils::selectChannel(_muxChannel)) {
                    vTaskDelay(_taskIntervalMs / portTICK_PERIOD_MS);
                    continue;
                }

                // Perform full sensor read while locked
                readRaw();
            }

            _lastReadDuration = micros() - readStart;
            _avgReadDuration = (_avgReadDuration * 7 + _lastReadDuration) / 8;
            _readCount++;
            _lastHeartbeat = millis();
            
            _currentInterval = Scheduler::instance().computeInterval(this, _mutexWaitTime);
            vTaskDelay(_currentInterval / portTICK_PERIOD_MS);
    }
}

    

};
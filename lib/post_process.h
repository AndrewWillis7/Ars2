#pragma once
#include <Arduino.h>
#include "scheduler.h"

class PostProcess {
public:
    friend class Scheduler;

    PostProcess(const char* name) :
        _name(name),
        _taskHandle(nullptr),
        _taskIntervalMs(20)
    {
        Scheduler::instance().registerPostProcess(this);
    }

    virtual ~PostProcess() {}

    virtual void setup() = 0;

    virtual void runOnce() = 0;

    void updateBlocking() {runOnce();}

    void startTask(uint32_t intervalMs = 20, BaseType_t core = tskNO_AFFINITY) {
        _taskIntervalMs = intervalMs;
        xTaskCreatePinnedToCore(
            _taskEntry, _name, 4096, this, 1, &_taskHandle, core
        );
    }

    bool taskRunning() const {return _taskHandle != nullptr;}

    void stopTask() {
        if (_taskHandle) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
    }

    void pauseTask() {_paused = true;}
    void resumeTask() {_paused = false;}
    bool isPaused() const {return _paused;}

    // Debug Interface
    virtual void debugPrint() {
        Serial.printf("[%s] No debugPrint override.\n", _name);
    }

    bool isAlive() const {
        return (millis() - _lastHeartbeat < 2000);
    }

    void printStats() {
        Serial.printf(
            "[%s] core=%u exec=%u last=%uus avg=%uus hb=%ums\n",
            _name,
            _taskCore,
            _runCount,
            _lastExecDuration,
            _avgExecDuration,
            millis() - _lastHeartbeat
        );
    }

protected:
    const char* _name;

    TaskHandle_t _taskHandle;
    volatile bool _paused = false;

    uint32_t _taskIntervalMs;
    uint32_t _taskCore = 0;

    uint32_t _lastExecDuration = 0;
    uint32_t _avgExecDuration = 0;
    uint32_t _runCount = 0;
    uint32_t _lastHeartbeat = 0;
    uint32_t _currentInterval = 50;
    uint32_t _minInterval = 10;
    uint32_t _maxInterval = 200;

private:
    static void _taskEntry(void* ptr) {
        reinterpret_cast<PostProcess*>(ptr)->taskLoop();
    }

    void taskLoop() {
        _taskCore = xPortGetCoreID();

            for(;;) {
                if (_paused) {
                    vTaskDelay(5 / portTICK_PERIOD_MS);
                    continue;
                }
                
                uint32_t t0 = micros();

                // No I2C lock
                runOnce();

                _lastExecDuration = micros() - t0;
                _avgExecDuration = (_avgExecDuration * 7 + _lastExecDuration) / 8;
                _runCount++;
                _lastHeartbeat = millis();

                _currentInterval = Scheduler::instance().computeInterval(this, 10);
                vTaskDelay(_taskIntervalMs / portTICK_PERIOD_MS);
            }
        }
};
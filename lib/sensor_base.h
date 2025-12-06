#pragma once
#include <Arduino.h>
#include <I2CUtils.h>

class SensorBase {
public:
    SensorBase(const char* name, uint8_t muxChannel) :
        _name(name), 
        _muxChannel(muxChannel),
        _taskHandle(nullptr), 
        _taskIntervalMs(50) 
    {}
    
    virtual ~SensorBase() {}

    // -- Setup --
    virtual void setup() = 0;

    // -- Blocking Aqui --
    virtual void readRaw() = 0;
    virtual void readBlocking() {
        I2CUtils::i2cLock();
        selectMux();
        readRaw();
        I2CUtils::i2cUnlock();

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

    // Stop running task
    void stopTask() {
        if (_taskHandle) {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
    }

    // -- Debug --
    virtual void debugPrint() {
        Serial.printf("[%s] No debugPrint override.\n", _name);
    }
    
protected:
    const char* _name;
    uint8_t _muxChannel;
    TaskHandle_t _taskHandle;
    uint32_t _taskIntervalMs;
    uint32_t _lastReadTime = 0;

    void selectMux() {
        if (!I2CUtils::selectChannel(_muxChannel)) {
            Serial.printf("[%s] Failed to select myx channel %d\n",
                _name, _muxChannel);
        }
    }

private:
    // static call for FreeRTOS
    static void _taskEntry(void* ptr) {
        reinterpret_cast<SensorBase*>(ptr)->taskLoop();
    }

    void taskLoop() {
        for (;;) {
            I2CUtils::i2cLock();
            selectMux();
            readRaw(); // We end up using the blocking aqui cause it doesnt matter based on setup
            I2CUtils::i2cUnlock();

            _lastReadTime = millis();
            vTaskDelay(_taskIntervalMs / portTICK_PERIOD_MS);
        }
    }
};
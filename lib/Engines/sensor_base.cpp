#include "sensor_base.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

SensorBase::SensorBase(const char* name, uint8_t muxChannel)
    : _nameStorage(name),
      _name(_nameStorage.c_str()),
      _muxChannel(muxChannel),
      _taskHandle(nullptr),
      _taskIntervalMs(50) {
    Scheduler::instance().registerSensor(this);
}

SensorBase::~SensorBase() {
    stopTask();
}

void SensorBase::readBlocking() {
    I2CUtils::ScopedI2C guard(_muxChannel);
    if (!guard.ok()) {
        return;
    }

    readRaw();
    _lastReadTime = millis();
}

void SensorBase::updateBlocking() {
    readBlocking();
}

void SensorBase::startTask(
    uint32_t intervalMs,
    BaseType_t core,
    UBaseType_t priority,
    uint32_t stackWords) {
    if (_taskHandle) {
        return;
    }

    _taskIntervalMs = intervalMs;
    _currentInterval = intervalMs;

    BaseType_t result = xTaskCreatePinnedToCore(
        _taskEntry,
        _name,
        stackWords,
        this,
        priority,
        &_taskHandle,
        core);

    if (result != pdPASS) {
        _taskHandle = nullptr;
        Serial.printf("[%s] Failed to start task\n", _name);
    }
}

bool SensorBase::taskRunning() const {
    return _taskHandle != nullptr;
}

uint32_t SensorBase::readCount() const {
    return _readCount;
}

bool SensorBase::consumeUpdate(uint32_t& lastSeen) const {
    uint32_t now = _readCount;
    if (now == lastSeen) {
        return false;
    }

    lastSeen = now;
    return true;
}

void SensorBase::stopTask() {
    TaskHandle_t handle = _taskHandle;
    if (!handle) {
        return;
    }

    _taskHandle = nullptr;

    if (handle == xTaskGetCurrentTaskHandle()) {
        vTaskDelete(nullptr);
    } else {
        vTaskDelete(handle);
    }
}

void SensorBase::pauseTask() {
    _paused = true;
}

void SensorBase::resumeTask() {
    _paused = false;
}

bool SensorBase::isPaused() const {
    return _paused;
}

void SensorBase::debugPrint() {
    Serial.printf("[%s] No debugPrint override.\n", _name);
}

bool SensorBase::isAlive() const {
    return (millis() - _lastHeartbeat) < 2000;
}

void SensorBase::printStats() {
    Serial.printf(
        "[%s] core=%u reads=%u last=%uus avg=%uus mutexWait=%uus hb=%ums\n",
        _name,
        _taskCore,
        _readCount,
        _lastReadDuration,
        _avgReadDuration,
        _mutexWaitTime,
        millis() - _lastHeartbeat);
}

void SensorBase::_taskEntry(void* ptr) {
    reinterpret_cast<SensorBase*>(ptr)->taskLoop();
}

void SensorBase::taskLoop() {
    _taskCore = xPortGetCoreID();

    for (;;) {
        if (_paused) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        const uint32_t readStart = micros();
        const uint32_t waitStart = micros();

        I2CUtils::i2cLock();
        _mutexWaitTime = micros() - waitStart;

        const bool channelReady = I2CUtils::selectChannel(_muxChannel);
        if (channelReady) {
            readRaw();
            _lastReadTime = millis();
        }

        I2CUtils::i2cUnlock();

        if (!channelReady) {
            vTaskDelay(pdMS_TO_TICKS(_taskIntervalMs));
            continue;
        }

        _lastReadDuration = micros() - readStart;
        _avgReadDuration = (_avgReadDuration * 7 + _lastReadDuration) / 8;
        ++_readCount;
        _lastHeartbeat = millis();

        _currentInterval = Scheduler::instance().computeInterval(this, _mutexWaitTime);
        vTaskDelay(pdMS_TO_TICKS(_currentInterval));
    }
}

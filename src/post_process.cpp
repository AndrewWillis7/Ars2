#include "../lib/post_process.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

PostProcess::PostProcess(const char* name)
    : _name(name),
      _taskHandle(nullptr),
      _taskIntervalMs(20) {
    Scheduler::instance().registerPostProcess(this);
}

PostProcess::~PostProcess() {
    stopTask();
}

void PostProcess::updateBlocking() {
    runOnce();
}

void PostProcess::startTask(
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

bool PostProcess::taskRunning() const {
    return _taskHandle != nullptr;
}

void PostProcess::stopTask() {
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

void PostProcess::pauseTask() {
    _paused = true;
}

void PostProcess::resumeTask() {
    _paused = false;
}

bool PostProcess::isPaused() const {
    return _paused;
}

void PostProcess::debugPrint() {
    Serial.printf("[%s] No debugPrint override.\n", _name);
}

bool PostProcess::isAlive() const {
    return (millis() - _lastHeartbeat) < 2000;
}

void PostProcess::printStats() {
    Serial.printf(
        "[%s] core=%u exec=%u last=%uus avg=%uus hb=%ums\n",
        _name,
        _taskCore,
        _runCount,
        _lastExecDuration,
        _avgExecDuration,
        millis() - _lastHeartbeat);
}

void PostProcess::_taskEntry(void* ptr) {
    reinterpret_cast<PostProcess*>(ptr)->taskLoop();
}

void PostProcess::taskLoop() {
    _taskCore = xPortGetCoreID();

    for (;;) {
        if (_paused) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        const uint32_t start = micros();

        runOnce();

        _lastExecDuration = micros() - start;
        _avgExecDuration = (_avgExecDuration * 7 + _lastExecDuration) / 8;
        ++_runCount;
        _lastHeartbeat = millis();

        _currentInterval = Scheduler::instance().computeInterval(this, 10);
        vTaskDelay(pdMS_TO_TICKS(_currentInterval));
    }
}

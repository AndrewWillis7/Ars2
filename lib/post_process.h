#pragma once

#include <Arduino.h>
#include "scheduler.h"

class PostProcess {
public:
    friend class Scheduler;

    explicit PostProcess(const char* name);
    virtual ~PostProcess();

    virtual void setup() = 0;
    virtual void runOnce() = 0;

    void updateBlocking();
    void startTask(
        uint32_t intervalMs = 20,
        BaseType_t core = tskNO_AFFINITY,
        UBaseType_t priority = 1,
        uint32_t stackWords = 4096);
    bool taskRunning() const;
    void stopTask();
    void pauseTask();
    void resumeTask();
    bool isPaused() const;
    virtual void debugPrint();
    bool isAlive() const;
    void printStats();

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
    static void _taskEntry(void* ptr);
    void taskLoop();
};

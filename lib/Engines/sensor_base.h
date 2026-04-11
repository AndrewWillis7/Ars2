#pragma once

#include <Arduino.h>
#include <I2CUtils.h>
#include "../scheduler.h"

class SensorBase {
public:
    friend class Scheduler;

    SensorBase(const char* name, uint8_t muxChannel);
    virtual ~SensorBase();

    virtual void setup() = 0;
    virtual void readRaw() = 0;
    virtual void readBlocking();

    void updateBlocking();
    void startTask(
        uint32_t intervalMs = 20,
        BaseType_t core = tskNO_AFFINITY,
        UBaseType_t priority = 1,
        uint32_t stackWords = 4096);
    bool taskRunning() const;

    uint32_t readCount() const;
    bool consumeUpdate(uint32_t& lastSeen) const;

    void stopTask();
    void pauseTask();
    void resumeTask();
    bool isPaused() const;

    virtual void debugPrint();
    bool isAlive() const;
    void printStats();

protected:
    String _nameStorage;
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
    static void _taskEntry(void* ptr);
    void taskLoop();
};

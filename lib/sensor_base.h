#pragma once
#include <Arduino.h>
#include <I2CUtils.h>
#include <TelemetryPacket.h>
#include "scheduler.h"

/*
Sensor Base

Abstract foundation for all I2C driven sensors in the arsenal
Each sensor will:
 * Identify itself with a name
 * Live on a specific TCA9548A mux Channel
 * Provides a blocking 'readRaw()' implementation
 * Also runs its own FreeRTOS task for data acquisition

Core Principles:
 * All I2C access is mutex-protected through I2CUtils
 * readRaw() is always executed when the channel is selected
 * Scheduler computes a dynamic per-sensor interval based on the mutex pressure
 * Sensors send a periodic "heartbeat" to verify life
*/

class SensorBase {
public:
    friend class Scheduler;

    /*
    Constructor

    Registers the new sensor with the global schedular instance.
    The mux channel determines which TCA9548A port is selected
    */
    SensorBase(const char* name, uint8_t muxChannel) :
        _name(name), 
        _muxChannel(muxChannel),
        _taskHandle(nullptr), 
        _taskIntervalMs(50) 
    {
        Scheduler::instance().registerSensor(this);
    }
    
    virtual ~SensorBase() {}

    // -----------------------------------------------------------------------
    // SETUP
    // -----------------------------------------------------------------------

    /*
    called once during system init.
    Child classes must handle I2C begin(), presence checks, and config
    */
    virtual void setup() = 0;

    // -----------------------------------------------------------------------
    // BLOCKING AQUISITION
    // -----------------------------------------------------------------------

    /*
    readRaw()
    
    Actual sensor transaction: perform I2C reads, update fields.
    Must be implemented for sensors to work, derived by the child
    This function MUST NOT select channels or manage locks
    */
    virtual void readRaw() = 0;

    /*
    readBlocking()

    Safe synchronous sensor read for the main loop or for debug
    This:
     * Takes the global I2C Mutex
     * selects the mux channel for this sensor
     * calls readRaw()
    */
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

    // -----------------------------------------------------------------------
    // FreeRTOS task Acquisition Path
    // -----------------------------------------------------------------------

    /*
    startTask()

    Launches a periodic acquisition task.
    intervalMs - desired nominal read interval
    core - CPU core affinity (literally core 1 or 0)
    */
    void startTask(uint32_t intervalMs = 20, BaseType_t core = tskNO_AFFINITY) {
        _taskIntervalMs = intervalMs;
        xTaskCreatePinnedToCore(
            _taskEntry, _name, 4096, this, 1, &_taskHandle, core
        );
    }

    bool taskRunning() const {return _taskHandle != nullptr;}

    // Sampling Helpers
    uint32_t readCount() const { return _readCount; }

    // Returns true exactly once per new sample
    bool consumeUpdate(uint32_t &lastSeen) const {
        uint32_t now = _readCount;
        if (now == lastSeen) return false;
        lastSeen = now;
        return true;
    }

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

    TelemetryTag _tag = TelemetryTag::NONE;

private:
    // static call for FreeRTOS
    static void _taskEntry(void* ptr) {
        reinterpret_cast<SensorBase*>(ptr)->taskLoop();
    }

    /*
    taskLoop()

    Main asynchronous loop
    Each iteration:
        1. Waits if paused
        2. Locks I2C bus (scopedLock ideally)
        3. selects mux channel
        4. Performs readRaw() under lock
        5. Updates timing statistics
        6. Computes next interval via Scheduler
        7. Sleeps
    
    Locking around the entire transaction ensures the sensor read
    with respect to other I2C devices
    */
    void taskLoop() {
        _taskCore = xPortGetCoreID();

        for (;;) { // We use for (;;) because it is intended to NEVER end unless no power, this is an embedded systems concept derived from C
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

            // Updates to the stats
            _lastReadDuration = micros() - readStart;
            _avgReadDuration = (_avgReadDuration * 7 + _lastReadDuration) / 8;
            _readCount++;
            _lastHeartbeat = millis();
            
            _currentInterval = Scheduler::instance().computeInterval(this, _mutexWaitTime);
            vTaskDelay(_currentInterval / portTICK_PERIOD_MS);
    }
}

    

};
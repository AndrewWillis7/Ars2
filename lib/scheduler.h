#pragma once
#include <Arduino.h>
#include <vector>

class SensorBase; //  forward Declaration
class PostProcess; // forward Declaration

class Scheduler {
public:
    // Singleton accessor
    static Scheduler& instance() {
        static Scheduler inst;
        return inst;
    }

    void registerSensor(SensorBase* sensor) {
        sensors.push_back(sensor);
    }

    void registerPostProcess(PostProcess* process) {
        processes.push_back(process);
    }

    void setBusThresholds(uint32_t idleUs, uint32_t congestedUs) {
        BUS_IDLE = idleUs;
        BUS_CONGESTED = congestedUs;
    }

    // Called by SensorBase per cycle
    uint32_t computeInterval(SensorBase* sensor, uint32_t muteWaitUs);

    // Called by PostProcess per cycle
    uint32_t computeInterval(PostProcess* process, uint32_t delay);

private:
    Scheduler() = default;

    std::vector<SensorBase*> sensors;
    std::vector<PostProcess*> processes;

    uint32_t BUS_IDLE = 50;
    uint32_t BUS_CONGESTED = 500;
};
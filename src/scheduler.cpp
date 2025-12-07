#include "../lib/scheduler.h"
#include "../lib/sensor_base.h"
#include "../lib/post_process.h"

uint32_t Scheduler::computeInterval(
    SensorBase* sensor,
    uint32_t waitUs)
{
    uint32_t interval = sensor->_currentInterval;

    if (waitUs > BUS_CONGESTED) {
        interval = min(interval + 5, sensor->_maxInterval);
    }
    else if (waitUs < BUS_IDLE) {
        interval = max(interval - 5, sensor->_minInterval);
    }

    return interval;
}

uint32_t Scheduler::computeInterval(
    PostProcess* process,
    uint32_t delay)
{
    uint32_t interval = process->_currentInterval;

    if (delay > BUS_CONGESTED) {
        interval = min(interval + 5, process->_maxInterval);
    }
    else if (delay < BUS_IDLE) {
        interval = max(interval - 5, process->_minInterval);
    }

    return interval;
}
#include "../lib/scheduler.h"
#include "../lib/sensor_base.h"

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
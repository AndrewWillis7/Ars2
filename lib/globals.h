#pragma once
#include <Arduino.h>
#include <vector>



namespace globals {

enum class SystemState {
    WAIT_CONFIG,
    RUNNING
};

struct SensorConfig {
    String name;
    String type;
    uint8_t port;
};

static SystemState state = SystemState::WAIT_CONFIG;

extern std::vector<SensorConfig> sensors;

extern std::vector<float> offsets;

inline void reserveSensors(size_t maxSensors) {
    sensors.reserve(maxSensors);
    offsets.reserve(6);
}

}
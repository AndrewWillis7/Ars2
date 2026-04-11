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

struct LightConfig {
    String name;
    uint8_t gpio;
    uint16_t count;
};

static SystemState state = SystemState::WAIT_CONFIG;

extern std::vector<SensorConfig> sensors;

extern std::vector<float> offsets;

inline void reserveSensors(size_t maxSensors) {
    sensors.reserve(maxSensors);
    offsets.reserve(6);
}

enum class LightPattern : uint8_t {
    OFF = 0,
    CONST,
    BLINK,
    PULSE,
    RAINBOW
};

struct LightState 
{
    String name;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t brightness = 255;
    LightPattern pattern = LightPattern::OFF;
    uint16_t periodMs = 1000;
};

extern std::vector<LightConfig> lights;
extern std::vector<LightState> lightStates;
extern bool lightsConfigured;

static globals::LightPattern parseLightPattern(String s) {
    s.trim();
    s.toUpperCase();

    if (s == "OFF")     return globals::LightPattern::OFF;
    if (s == "CONST")   return globals::LightPattern::CONST;
    if (s == "BLINK")   return globals::LightPattern::BLINK;
    if (s == "PULSE")   return globals::LightPattern::PULSE;
    if (s == "RAINBOW") return globals::LightPattern::RAINBOW;

    return globals::LightPattern::OFF;
}

inline void reserveLights(size_t maxCount) {
    lights.reserve(maxCount);
    lightStates.reserve(maxCount);
}

}
#pragma once

#include <Arduino.h>

#include <array>
#include <vector>

namespace RuntimeConfig {

enum class SystemState : uint8_t {
    WAIT_CONFIG,
    RUNNING
};

struct SensorConfig {
    String name;
    String type;
    uint8_t port = 0;
};

struct LightConfig {
    String name;
    uint8_t gpio = 0;
    uint16_t count = 0;
};

enum class LightPattern : uint8_t {
    OFF = 0,
    CONST,
    BLINK,
    PULSE,
    RAINBOW
};

struct LightState {
    String name;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t brightness = 255;
    LightPattern pattern = LightPattern::OFF;
    uint16_t periodMs = 1000;
};

void begin();

void reserveSensors(size_t maxSensors);
void reserveLights(size_t maxCount);

SystemState state();
bool isRunning();
void setState(SystemState nextState);

void setSensorConfigs(const std::vector<SensorConfig>& configs);
std::vector<SensorConfig> sensorConfigs();

void setOffsets(const std::array<float, 6>& values);
std::array<float, 6> offsets();

bool hasLightConfig();
uint32_t lightConfigVersion();
void setLightConfigs(const std::vector<LightConfig>& configs);
std::vector<LightConfig> lightConfigs();
std::vector<LightState> lightStates();
bool updateLightState(const String& name, const LightState& state);

LightPattern parseLightPattern(String text);

}  // namespace RuntimeConfig

#include "RuntimeConfig.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <hw_config.h>

namespace {

SemaphoreHandle_t g_configMutex = nullptr;
volatile RuntimeConfig::SystemState g_state = RuntimeConfig::SystemState::WAIT_CONFIG;
std::vector<RuntimeConfig::SensorConfig> g_sensors;
std::array<float, 6> g_offsets = {
    OFF_1_X, OFF_1_Y, OFF_1_H,
    OFF_2_X, OFF_2_Y, OFF_2_H,
};
std::vector<RuntimeConfig::LightConfig> g_lights;
std::vector<RuntimeConfig::LightState> g_lightStates;
uint32_t g_lightConfigVersion = 0;

void ensureMutex() {
    if (!g_configMutex) {
        g_configMutex = xSemaphoreCreateMutex();
    }
}

class ScopedConfigLock {
public:
    ScopedConfigLock() {
        ensureMutex();
        xSemaphoreTake(g_configMutex, portMAX_DELAY);
    }

    ~ScopedConfigLock() {
        xSemaphoreGive(g_configMutex);
    }
};

RuntimeConfig::LightState makeDefaultLightState(const RuntimeConfig::LightConfig& cfg) {
    RuntimeConfig::LightState state;
    state.name = cfg.name;
    return state;
}

}  // namespace

namespace RuntimeConfig {

void begin() {
    ensureMutex();

    ScopedConfigLock lock;
    g_state = SystemState::WAIT_CONFIG;
    g_sensors.clear();
    g_offsets = {
        OFF_1_X, OFF_1_Y, OFF_1_H,
        OFF_2_X, OFF_2_Y, OFF_2_H,
    };
    g_lights.clear();
    g_lightStates.clear();
    g_lightConfigVersion = 0;
}

void reserveSensors(size_t maxSensors) {
    ScopedConfigLock lock;
    g_sensors.reserve(maxSensors);
}

void reserveLights(size_t maxCount) {
    ScopedConfigLock lock;
    g_lights.reserve(maxCount);
    g_lightStates.reserve(maxCount);
}

SystemState state() {
    ScopedConfigLock lock;
    return g_state;
}

bool isRunning() {
    return state() == SystemState::RUNNING;
}

void setState(SystemState nextState) {
    ScopedConfigLock lock;
    g_state = nextState;
}

void setSensorConfigs(const std::vector<SensorConfig>& configs) {
    ScopedConfigLock lock;
    g_sensors = configs;
}

std::vector<SensorConfig> sensorConfigs() {
    ScopedConfigLock lock;
    return g_sensors;
}

void setOffsets(const std::array<float, 6>& values) {
    ScopedConfigLock lock;
    g_offsets = values;
}

std::array<float, 6> offsets() {
    ScopedConfigLock lock;
    return g_offsets;
}

bool hasLightConfig() {
    ScopedConfigLock lock;
    return !g_lights.empty();
}

uint32_t lightConfigVersion() {
    ScopedConfigLock lock;
    return g_lightConfigVersion;
}

void setLightConfigs(const std::vector<LightConfig>& configs) {
    ScopedConfigLock lock;

    g_lights = configs;
    g_lightStates.clear();
    g_lightStates.reserve(g_lights.size());

    for (const auto& cfg : g_lights) {
        g_lightStates.push_back(makeDefaultLightState(cfg));
    }

    ++g_lightConfigVersion;
}

std::vector<LightConfig> lightConfigs() {
    ScopedConfigLock lock;
    return g_lights;
}

std::vector<LightState> lightStates() {
    ScopedConfigLock lock;
    return g_lightStates;
}

bool updateLightState(const String& name, const LightState& state) {
    ScopedConfigLock lock;

    for (auto& current : g_lightStates) {
        if (current.name.equalsIgnoreCase(name)) {
            current = state;
            current.name = name;
            return true;
        }
    }

    return false;
}

LightPattern parseLightPattern(String text) {
    text.trim();
    text.toUpperCase();

    if (text == "CONST") {
        return LightPattern::CONST;
    }
    if (text == "BLINK") {
        return LightPattern::BLINK;
    }
    if (text == "PULSE") {
        return LightPattern::PULSE;
    }
    if (text == "RAINBOW") {
        return LightPattern::RAINBOW;
    }

    return LightPattern::OFF;
}

}  // namespace RuntimeConfig

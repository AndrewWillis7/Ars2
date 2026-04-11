#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <RuntimeConfig.h>

#include <vector>

class LightingEngine {
public:
    LightingEngine();
    ~LightingEngine();

    void begin(const std::vector<RuntimeConfig::LightConfig>& configs);
    void update(const std::vector<RuntimeConfig::LightState>& states);

    bool isInitialized() const { return _initialized; }

private:
    struct StripRuntime
    {
        String name;
        uint8_t gpio;
        uint16_t count;
        Adafruit_NeoPixel* strip = nullptr;
    };

    std::vector<StripRuntime> _strips;
    bool _initialized = false;

    const RuntimeConfig::LightState* findState(
        const std::vector<RuntimeConfig::LightState>& states,
        const String& name) const;

    void clearStrips();
    void renderStrip(StripRuntime& rt, const RuntimeConfig::LightState& st, uint32_t now);

    void renderOff(StripRuntime& rt);
    void renderConst(StripRuntime& rt, const RuntimeConfig::LightState& st);
    void renderBlink(StripRuntime& rt, const RuntimeConfig::LightState& st, uint32_t now);
    void renderPulse(StripRuntime& rt, const RuntimeConfig::LightState& st, uint32_t now);
    void renderRainbow(StripRuntime& rt, const RuntimeConfig::LightState& st, uint32_t now);

    uint8_t scale8(uint8_t value, uint8_t brightness);
    uint32_t makeColor(Adafruit_NeoPixel* strip, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    uint32_t wheel(Adafruit_NeoPixel* strip, uint8_t pos);
};

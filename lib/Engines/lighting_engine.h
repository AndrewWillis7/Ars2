#pragma once

#include <Arduino.h>
#include <vector>
#include <Adafruit_NeoPixel.h>
#include "../lib/globals.h"

class LightingEngine {
public:
    LightingEngine();

    void begin();
    void update();

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

    globals::LightState* findState(const String& name);
    
    void renderStrip(StripRuntime& rt, globals::LightState& st, uint32_t now);

    void renderOff(StripRuntime& rt);
    void renderConst(StripRuntime& rt, globals::LightState& st);
    void renderBlink(StripRuntime& rt, globals::LightState& st, uint32_t now);
    void renderPulse(StripRuntime& rt, globals::LightState& st, uint32_t now);
    void renderRainbow(StripRuntime& rt, globals::LightState& st, uint32_t now);

    uint8_t scale8(uint8_t value, uint8_t brightness);
    uint32_t makeColor(Adafruit_NeoPixel* strip, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    uint32_t wheel(Adafruit_NeoPixel* strip, uint8_t pos);
};
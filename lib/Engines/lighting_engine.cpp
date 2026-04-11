#include "lighting_engine.h"

LightingEngine::LightingEngine() {}

void LightingEngine::begin() {
    // Prevent duplicate begin calls from stacking allocated strips
    for (auto& rt : _strips) {
        if (rt.strip) {
            delete rt.strip;
            rt.strip = nullptr;
        }
    }
    _strips.clear();

    if (globals::lights.empty()) {
        Serial.println("[LIGHT] No light configs present, begin skipped");
        _initialized = false;
        return;
    }

    for (auto& cfg : globals::lights) {
        StripRuntime rt;
        rt.name = cfg.name;
        rt.gpio = cfg.gpio;
        rt.count = cfg.count;

        rt.strip = new Adafruit_NeoPixel(cfg.count, cfg.gpio, NEO_GRB + NEO_KHZ800);
        if (!rt.strip) {
            Serial.printf("[LIGHT] Failed alloc for %s\n", rt.name.c_str());
            continue;
        }

        rt.strip->begin();
        rt.strip->clear();
        rt.strip->show();

        _strips.push_back(rt);

        Serial.printf("[LIGHT] Started strip %s on GPIO %u with %u LEDs\n",
                      rt.name.c_str(),
                      rt.gpio,
                      rt.count);
    }

    _initialized = !_strips.empty();
}

void LightingEngine::update() {
    if (!_initialized) return;

    uint32_t now = millis();

    for (auto& rt : _strips) {
        globals::LightState* st = findState(rt.name);
        if (!st || !rt.strip) continue;

        renderStrip(rt, *st, now);
    }
}

globals::LightState* LightingEngine::findState(const String& name) {
    for (auto& st : globals::lightStates) {
        if (st.name == name) {
            return &st;
        }
    }
    return nullptr;
}

void LightingEngine::renderStrip(StripRuntime& rt, globals::LightState& st, uint32_t now) {
    switch (st.pattern) {
        case globals::LightPattern::OFF:
            renderOff(rt);
            break;

        case globals::LightPattern::CONST:
            renderConst(rt, st);
            break;

        case globals::LightPattern::BLINK:
            renderBlink(rt, st, now);
            break;

        case globals::LightPattern::PULSE:
            renderPulse(rt, st, now);
            break;

        case globals::LightPattern::RAINBOW:
            renderRainbow(rt, st, now);
            break;

        default:
            renderOff(rt);
            break;
    }
}

void LightingEngine::renderOff(StripRuntime& rt) {
    rt.strip->clear();
    rt.strip->show();
}

void LightingEngine::renderConst(StripRuntime& rt, globals::LightState& st) {
    uint32_t c = makeColor(rt.strip, st.r, st.g, st.b, st.brightness);

    for (uint16_t i = 0; i < rt.count; i++) {
        rt.strip->setPixelColor(i, c);
    }

    rt.strip->show();
}

void LightingEngine::renderBlink(StripRuntime& rt, globals::LightState& st, uint32_t now) {
    uint16_t period = (st.periodMs == 0) ? 1 : st.periodMs;
    bool on = ((now / period) % 2) == 0;

    uint32_t c = on ? makeColor(rt.strip, st.r, st.g, st.b, st.brightness) : 0;

    for (uint16_t i = 0; i < rt.count; i++) {
        rt.strip->setPixelColor(i, c);
    }

    rt.strip->show();
}

void LightingEngine::renderPulse(StripRuntime& rt, globals::LightState& st, uint32_t now) {
    uint16_t period = (st.periodMs == 0) ? 1 : st.periodMs;

    float phase = (float)(now % period) / (float)period;
    float wave = 0.5f - 0.5f * cosf(phase * 2.0f * PI); // smooth 0..1
    uint8_t dynBrightness = (uint8_t)(wave * st.brightness);

    uint32_t c = makeColor(rt.strip, st.r, st.g, st.b, dynBrightness);

    for (uint16_t i = 0; i < rt.count; i++) {
        rt.strip->setPixelColor(i, c);
    }

    rt.strip->show();
}

void LightingEngine::renderRainbow(StripRuntime& rt, globals::LightState& st, uint32_t now) {
    (void)st; // currently brightness not applied to rainbow here

    uint8_t offset = (now / 20) & 0xFF;

    for (uint16_t i = 0; i < rt.count; i++) {
        uint8_t pos = ((i * 256 / rt.count) + offset) & 0xFF;
        rt.strip->setPixelColor(i, wheel(rt.strip, pos));
    }

    rt.strip->show();
}

uint8_t LightingEngine::scale8(uint8_t value, uint8_t brightness) {
    return (uint16_t)value * brightness / 255;
}

uint32_t LightingEngine::makeColor(Adafruit_NeoPixel* strip, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    return strip->Color(
        scale8(r, brightness),
        scale8(g, brightness),
        scale8(b, brightness)
    );
}

uint32_t LightingEngine::wheel(Adafruit_NeoPixel* strip, uint8_t pos) {
    pos = 255 - pos;

    if (pos < 85) {
        return strip->Color(255 - pos * 3, 0, pos * 3);
    }

    if (pos < 170) {
        pos -= 85;
        return strip->Color(0, pos * 3, 255 - pos * 3);
    }

    pos -= 170;
    return strip->Color(pos * 3, 255 - pos * 3, 0);
}
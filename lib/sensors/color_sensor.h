#pragma once
#include "../lib/sensor_base.h"
#include <Adafruit_TCS34725.h>

class ColorSensor : public SensorBase {
public:
    ColorSensor(const char* name, uint8_t channel)
        : SensorBase(name, channel),
          tcs(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X)
    {}

    void setup() override {
        I2CUtils::i2cLock();
        bool muxOK = I2CUtils::selectChannel(_muxChannel);
        I2CUtils::i2cUnlock();

        if (!muxOK) {
            Serial.printf("[%s] MUX select failed\n", _name);
            return;
        }

        if (!tcs.begin()) {
            Serial.printf("[%s] Color sensor not found!\n", _name);
            return;
        }
        
        Serial.printf("Color sensor on CH%u initialized OK\n", _muxChannel);
        uint8_t id = tcs.read8(TCS34725_ID);
        Serial.printf("CH%u: ID=0x%02X\n", _muxChannel, id);
    }

    void readRaw() override {
        tcs.getRawData(&red, &green, &blue, &clear);

        TelemetryPacket p{};
        p.tag = (_muxChannel == HW_SC_CS1) ? TelemetryTag::CSA : TelemetryTag::CSB;
        p.a = red;
        p.b = green;
        p.c = blue;
        p.ms = millis();
        TelemetryBus::publish(p);
    }

    void debugPrint() override {
        Serial.printf("[%s] R=%u G=%u B=%u C=%u\n",
            _name, red, green, blue, clear);
    }

    uint16_t red, green, blue, clear;

private:
    Adafruit_TCS34725 tcs;
};
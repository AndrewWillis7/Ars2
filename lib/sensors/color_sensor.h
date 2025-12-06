#include "../lib/sensor_base.h"
#include <Adafruit_TCS34725.h>

class ColorSensor : public SensorBase {
public:
    ColorSensor(const char* name, uint8_t channel)
        : SensorBase(name, channel),
          tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X)
    {}

    void setup() override {
        I2CUtils::ScopedI2C guard(_muxChannel);
        if (!guard.ok()) {
            Serial.printf("[%s] MUX select failed\n", _name);
            return;
        }

        if (!tcs.begin()) {
            Serial.printf("[%s] Color sensor not found!\n", _name);
            return;
        }
    }

    void readRaw() override {
        tcs.getRawData(&red, &green, &blue, &clear);
    }

    void debugPrint() override {
        Serial.printf("[%s] R=%u G=%u B=%u C=%u\n",
            _name, red, green, blue, clear);
    }

    uint16_t red, green, blue, clear;

private:
    Adafruit_TCS34725 tcs;
};
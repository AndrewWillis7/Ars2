#pragma once
#include "../lib/sensor_base.h"
#include <SparkFun_Qwiic_OTOS_Arduino_Library.h>

class OpticalSensor : public SensorBase {
public:
    OpticalSensor(const char* name, uint8_t channel, float offsetX, float offsetY, float offsetH)
        : SensorBase(name, channel), off_x(offsetX), off_y(offsetY), off_h(offsetH)
    {}

    void setup() override {
        I2CUtils::i2cLock();
        bool muxOK = I2CUtils::selectChannel(_muxChannel);
        I2CUtils::i2cUnlock();

        if (!muxOK) {
            Serial.printf("[%s] MUX select failed\n", _name);
            return;
        }

        if (!otos.begin()) {
            Serial.printf("[%s] OTOS not found!\n", _name);
            return;
        }

        Serial.printf("OTOS on CH%u initialized OK\n", _muxChannel);

        offset.x = off_x;
        offset.y = off_y;
        offset.h = off_h;

        otos.setOffset(offset);
        otos.calibrateImu();
        otos.resetTracking();
    }

    void readRaw() override {
        otos.getPosition(pos);
    }

    // Wacky Print statement so we dont get the encoding error
    void debugPrint() override {
        Serial.printf("[%s] ", _name);
        Serial.print("X=");
        Serial.print(pos.x);
        Serial.print("Y=");
        Serial.print(pos.y);
        Serial.print("H=");
        Serial.print(pos.h);
        Serial.print("\n");
    }

    sfe_otos_pose2d_t pos;
private:
    float off_x;
    float off_y;
    float off_h;

    sfe_otos_pose2d_t offset;
    QwiicOTOS otos;
};
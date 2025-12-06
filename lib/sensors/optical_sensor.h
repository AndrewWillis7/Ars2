#pragma once
#include "../lib/sensor_base.h"
#include <SparkFun_Qwiic_OTOS_Arduino_Library.h>

class OpticalSensor : public SensorBase {
public:
    OpticalSensor(const char* name, uint8_t channel)
        : SensorBase(name, channel)
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
    QwiicOTOS otos;
};
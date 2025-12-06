#pragma once
#include "../lib/gpio_base.h"
#include <hw_config.h>

class EncoderSensor : public GPIOBase {
public:
    EncoderSensor(const char* name, uint8_t pin)
        : GPIOBase(name), data_pin(pin), value(0)
    {}

    void setup() override {
        pinMode(data_pin, INPUT_PULLUP);
        pinMode(clk_pin, OUTPUT);
        pinMode(cs_pin, OUTPUT);

        digitalWrite(clk_pin, LOW); // Set clock to 0 initially
        digitalWrite(cs_pin, HIGH); // Idle High for the Chip Select (Falling edge triggered)
    }

    void readRaw() override {
        uint16_t raw = 0;
        
        // Start Frame
        digitalWrite(cs_pin, HIGH);
        delayMicroseconds(1);
        digitalWrite(cs_pin, LOW);
        delayMicroseconds(1);

        // Read 10 bits, MSB first
        for (int i = 0; i < 10; i++) {
            digitalWrite(clk_pin, HIGH);
            delayMicroseconds(1);

            int bit = digitalRead(data_pin);
            raw |= (bit << (9 - i));

            digitalWrite(clk_pin, LOW);
            delayMicroseconds(1);
        }

        // Read 6 status + 1 parity discard
        for (int i = 0; i < 7; i++) {
            digitalWrite(clk_pin, HIGH);
            delayMicroseconds(1);
            digitalWrite(clk_pin, LOW);
            delayMicroseconds(1);
        }

        digitalWrite(cs_pin, HIGH);
        value = raw;
    }

    void debugPrint() override {
        Serial.printf("[%s] val=%d\n", _name, value);
    }

    int value;
private:
    uint8_t data_pin;
    uint8_t clk_pin = HW_C_ENCLK;
    uint8_t cs_pin = HW_C_ENCS;
};
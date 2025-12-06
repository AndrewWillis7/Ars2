#include "I2CUtils.h"

namespace I2CUtils {

// Allocate the Mutex globally so it doesnt get multiple defs
SemaphoreHandle_t I2C_Mutex = nullptr;

uint32_t totalLocks = 0;
uint32_t totalUnlocks = 0;
uint32_t totalMutexWaits = 0;
volatile uint8_t currentChannel = 255;

void begin() {
    I2C_Mutex = xSemaphoreCreateMutex();
}

bool selectChannel(uint8_t ch) {
    if (ch == currentChannel) {
        return true; // Channel is set, no traffic
    }

    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << ch);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
        currentChannel = ch;
        return true;
    }
    return false;
}

bool ensureChannel(uint8_t ch) {
    if (currentChannel == ch) return true;
    return selectChannel(ch);
}

bool busIsStuck() {
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);

    bool sda = digitalRead(SDA_PIN);
    bool scl = digitalRead(SCL_PIN);

    return (sda == LOW || scl == LOW);
}

void printI2CStats() {
    Serial.printf(
        "[I2C] locks=%u unlocks=%u totalWait=%uus\n",
        totalLocks, totalUnlocks, totalMutexWaits
    );
}

// TODO, add Recovery if necessary, and guard on that recov

} // eol namespace


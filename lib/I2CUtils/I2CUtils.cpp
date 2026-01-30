#include "I2CUtils.h"

namespace I2CUtils {

// Allocate the Mutex globally so it doesnt get multiple defs
SemaphoreHandle_t I2C_Mutex = nullptr;

uint32_t totalLocks = 0;
uint32_t totalUnlocks = 0;
uint32_t totalMutexWaits = 0;
volatile uint8_t currentChannel = 255;

void begin() {
    if (!I2C_Mutex) {
        I2C_Mutex = xSemaphoreCreateMutex();
    }
    Wire.begin(SDA_PIN, SCL_PIN, 400000);
    currentChannel = 0xFF;
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

void scanI2C() {
    for (uint8_t ch = 0; ch < 8; ch++) {
        selectChannel(ch);
        delay(2); // let the bus settle

        //Serial.printf("Scanning mux channel %d\n", ch);

        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("  FOUND 0x%02X on channel %d\n", addr, ch);
            }
        }
    }

    Serial.println("SCAN COMPLETE!");
}

void selectMuxRaw(uint8_t ch) {
    Wire.beginTransmission(0x70);      // mux addr
    Wire.write(1 << ch);               // select channel
    uint8_t err = Wire.endTransmission();
    Serial.printf("selectMuxRaw ch=%u err=%u\n", ch, err);
}


bool ensureChannel(uint8_t ch) {
    if (currentChannel == ch) return true;

    xSemaphoreTake(I2C_Mutex, portMAX_DELAY);

    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << ch);
    uint8_t err = Wire.endTransmission();

    xSemaphoreGive(I2C_Mutex);

    if (err != 0) {
        Serial.printf("MUX SWITCH FAIL addr=0x%02X ch=%u mask=0x%02X err=%u\n", 
            MUX_ADDR, ch, (1 << ch), err);
        return false;
    }

    currentChannel = ch;
    return true;
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


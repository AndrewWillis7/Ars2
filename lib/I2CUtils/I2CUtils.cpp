#include "I2CUtils.h"

namespace I2CUtils {

// Allocate the Mutex globally so it doesnt get multiple defs
SemaphoreHandle_t I2C_Mutex = nullptr;

void begin() {
    I2C_Mutex = xSemaphoreCreateMutex();
}

bool selectChannel(uint8_t ch) {
    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << ch);
    return Wire.endTransmission() == 0;
}

} // eol namespace


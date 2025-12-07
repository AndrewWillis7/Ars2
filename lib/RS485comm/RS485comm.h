#include "Arduino.h"
#include <hw_config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace RS485comm {

static const uint8_t enablePin = COMM_EN_PIN;
static const char HEAD = '#';
static const char* FOOTER = "\r\n";

extern SemaphoreHandle_t RS485_Mutex;

extern uint32_t totalLocks;
extern uint32_t totalUnlocks;
extern uint32_t totalMutexWaits;
extern uint32_t bytesSent;
extern uint32_t packetsSent;

void enableTX();
void enableRX();

void begin(HardwareSerial& serial, uint32_t baud);

void sendRaw(const char* data);
void sendPacket(const char* payload);

// Mutex and Scoped Guard
inline void lock() {
    uint32_t t0 = micros();
    xSemaphoreTake(RS485_Mutex, portMAX_DELAY);
    totalLocks++;
    totalMutexWaits += (micros() - t0);
}

inline void unlock() {
    xSemaphoreGive(RS485_Mutex);
    totalUnlocks++;
}

class Scoped485 {
public:
    Scoped485() {
        lock();
        enableTX();
        delayMicroseconds(50); // RS485 driver settle time
    }

    ~Scoped485() {
        enableRX();
        unlock();
    }
};

void printStats();

} // eol namespace
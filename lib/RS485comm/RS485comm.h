#pragma once
#include "Arduino.h"
#include <hw_config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace RS485comm {

extern const uint8_t enablePin;
extern const char* FOOTER;

extern SemaphoreHandle_t RS485_Mutex;
extern HardwareSerial* serialPort;

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

void lock();     // now just declarations
void unlock();

class Scoped485 {
public:
    Scoped485(); 
    ~Scoped485();
};

void printStats();

} // namespace RS485comm

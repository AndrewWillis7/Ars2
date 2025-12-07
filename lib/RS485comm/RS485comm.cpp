#include "RS485comm.h"

namespace RS485comm {

HardwareSerial* serialPort = nullptr;
SemaphoreHandle_t RS485_Mutex = nullptr;

uint32_t totalLocks = 0;
uint32_t totalUnlocks = 0;
uint32_t totalMutexWaits = 0;
uint32_t bytesSent = 0;
uint32_t packetsSent = 0;

void enableTX() {
    digitalWrite(enablePin, HIGH);
}

void enableRX() {
    digitalWrite(enablePin, LOW);
}

void begin(HardwareSerial& serial, uint32_t baud) {
    serialPort = &serial;

    if (!RS485_Mutex) {
        RS485_Mutex = xSemaphoreCreateMutex();
    }

    pinMode(enablePin, OUTPUT);
    enableRX(); // recieve by default

    serialPort->begin(baud);
    delay(100);
}

void sendRaw(const char* data) {
    if (!serialPort) return;
    size_t len = strlen(data);
    serialPort->write((const uint8_t*)data, len);
    bytesSent += len;
}

void sendPacket(const char* payload) {
    Scoped485 guard; // takes mutex and enables TX

    if (!serialPort) return;

    serialPort->print(HEAD);
    serialPort->print(payload);
    serialPort->print(FOOTER);

    packetsSent++;
}

void printStats() {
    Serial.printf(
        "[RS485] locks=%u unlocks=%u wait=%uus bytes=%u packets=%u\n",
        totalLocks, totalUnlocks, totalMutexWaits, bytesSent, packetsSent
    );
}

} // eol namespace
#include "RS485comm.h"

namespace RS485comm {

const uint8_t enablePin = COMM_EN_PIN;
const char* FOOTER = "\r\n";

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
    enableRX();

    serialPort->begin(baud, SERIAL_8N1, COMM_RX_PIN, COMM_TX_PIN);
    delay(50);
}

void sendRaw(const char* data) {
    if (!serialPort || !data) {
        return;
    }

    Scoped485 guard;

    const size_t len = strlen(data);
    serialPort->write(reinterpret_cast<const uint8_t*>(data), len);
    bytesSent += len;
}

void sendPacket(const char* payload) {
    if (!serialPort || !payload) {
        return;
    }

    Scoped485 guard;

    serialPort->print(payload);
    serialPort->print(FOOTER);

    bytesSent += (strlen(payload) + strlen(FOOTER));
    ++packetsSent;
}

void lock() {
    const uint32_t t0 = micros();
    xSemaphoreTake(RS485_Mutex, portMAX_DELAY);
    ++totalLocks;
    totalMutexWaits += (micros() - t0);
}

void unlock() {
    xSemaphoreGive(RS485_Mutex);
    ++totalUnlocks;
}

Scoped485::Scoped485() {
    lock();
    enableTX();
    delayMicroseconds(50);
}

Scoped485::~Scoped485() {
    if (serialPort) {
        serialPort->flush();
    }

    delayMicroseconds(30);
    enableRX();
    unlock();
}

void printStats() {
    Serial.printf(
        "[RS485] locks=%u unlocks=%u wait=%uus bytes=%u packets=%u\n",
        totalLocks,
        totalUnlocks,
        totalMutexWaits,
        bytesSent,
        packetsSent);
}

}  // namespace RS485comm

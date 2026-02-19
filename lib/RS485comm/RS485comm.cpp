#include "RS485comm.h"

namespace RS485comm {

// ---------------------------
// GLOBAL DEFINITIONS
// ---------------------------

const uint8_t enablePin = COMM_EN_PIN;
const char* FOOTER = "\r\n";

HardwareSerial* serialPort = nullptr;
SemaphoreHandle_t RS485_Mutex = nullptr;

uint32_t totalLocks = 0;
uint32_t totalUnlocks = 0;
uint32_t totalMutexWaits = 0;
uint32_t bytesSent = 0;
uint32_t packetsSent = 0;

// ---------------------------
// LOW-LEVEL PIN CONTROL
// ---------------------------

void enableTX() {
    // DE/RE high → transmit mode
    digitalWrite(enablePin, HIGH);
}

void enableRX() {
    // DE/RE low → receive mode
    digitalWrite(enablePin, LOW);
}

// ---------------------------
// INITIALIZATION
// ---------------------------

void begin(HardwareSerial& serial, uint32_t baud) {
    serialPort = &serial;

    if (!RS485_Mutex) {
        RS485_Mutex = xSemaphoreCreateMutex();
    }
            Serial.println("Ping");
    pinMode(enablePin, OUTPUT);
    enableRX(); // idle state → receiver enabled

    serialPort->begin(baud, SERIAL_8N1, COMM_RX_PIN, COMM_TX_PIN);
    delay(50); // settle
}

// ---------------------------
// RAW SEND
// ---------------------------

void sendRaw(const char* data) {
    if (!serialPort || !data) {
        Serial.print("No Serial port found, or no data. . .");
        return;
    }

    //data += FOOTER;
    size_t len = strlen(data);
    serialPort->write(reinterpret_cast<const uint8_t*>(data), len);
    bytesSent += len;
    Serial.println("Packet has been sent");
}

// ---------------------------
// PACKET SEND
// ---------------------------

void sendPacket(const char* payload) {
    if (!serialPort || !payload) {
        Serial.print("No Serial Port found, or no data. . .");
        return;
    }
    serialPort->flush();

    Scoped485 guard; // mutex + TX enable

    serialPort->print(payload);
    serialPort->print(FOOTER);

    //debug 
    Serial.print("Sending: ");
    Serial.print(payload);
    Serial.print(FOOTER);

    bytesSent += (strlen(payload) + 3);

    packetsSent++;
    //Serial.println("Packet has been sent");
}

// ---------------------------
// MUTEX CONTROL
// ---------------------------

void lock() {
    uint32_t t0 = micros();
    xSemaphoreTake(RS485_Mutex, portMAX_DELAY);
    totalLocks++;
    totalMutexWaits += (micros() - t0);
}

void unlock() {
    xSemaphoreGive(RS485_Mutex);
    totalUnlocks++;
}

// ---------------------------
// SCOPED GUARD
// ---------------------------

Scoped485::Scoped485() {
    lock();
    enableTX();
    delayMicroseconds(50); // chip settle time
}

Scoped485::~Scoped485() {
    if (serialPort) {
        serialPort->flush();
    }

    delayMicroseconds(30);

    enableRX();
    unlock();
}

// ---------------------------
// DEBUG
// ---------------------------

void printStats() {
    Serial.printf(
        "[RS485] locks=%u unlocks=%u wait=%uus bytes=%u packets=%u\n",
        totalLocks,
        totalUnlocks,
        totalMutexWaits,
        bytesSent,
        packetsSent
    );
}

} // namespace RS485comm

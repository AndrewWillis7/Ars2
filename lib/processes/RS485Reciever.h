#pragma once
#include <Arduino.h>
#include <RS485comm.h>
#include "../lib/post_process.h"

class RS485Reciever : public PostProcess {
public:
    RS485Reciever() : PostProcess("RS485-RX") {}

    void setup() override {
        startTask(1, 1); // check every 1ms
        rxBuffer.reserve(128);
        Serial.println("Init RS485");
    }

protected:
    void runOnce() override {
        if (replying) return;

        HardwareSerial* port = RS485comm::serialPort;
        if (!port) return;

        while (port->available()) {
            char c = port->read();
            processIncoming(c);
        }
    }


private:
    String rxBuffer;
    bool inPacket = false;
    volatile bool replying = false;

    void processIncoming(char c) {
        if (!inPacket) {
            if (c == '#') {
                inPacket = true;
                rxBuffer = "";
            }
            Serial.println("Not In Packet");
            return;
        }

        if (c == '\n') {
            handlePacket(rxBuffer);
            inPacket = false;
            return;
        }

        if (c != '\r') {
            rxBuffer += c;
        }
    }

    void handlePacket(const String& packet) {
        Serial.printf("[RS485] RX: %s\n", packet.c_str());

        char txBuffer[128];  // must be big enough
        snprintf(txBuffer, sizeof(txBuffer), "<ACK>%s", packet.c_str());

        RS485comm::sendRaw(txBuffer);
    }

};
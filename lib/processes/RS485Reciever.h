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
        //Serial.println("Hardware Port Found");

        while (~replying) {
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
            //Serial.println("Not In Packet");
            return;
        }

        if (c == '\n') {
            handlePacket(rxBuffer);
            inPacket = false;
            return;
        }

        if (c == '\r') return;
        if ((uint8_t)c < 32 || (uint8_t)c > 126) return;
        rxBuffer += c;
    }

    void handlePacket(const String& packet) {
        replying = true;
        Serial.printf("[RS485] RX: %s\n", packet.c_str());

        char txBuffer[256];  // must be big enough
        snprintf(txBuffer, sizeof(txBuffer), "%s<ACK>", packet.c_str());

        RS485comm::enableTX(); // Shouldnt be needed :(
        RS485comm::sendPacket(txBuffer);
        Serial.printf("[RS485] TX: %s\n", txBuffer);
        replying = false;
    }

};
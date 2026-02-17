#pragma once
#include <Arduino.h>
#include <RS485comm.h>
#include "../lib/post_process.h"

#include <TelemetryBus.h>
#include "TelemetrySnapshot.h"

class RS485Transceiver : public PostProcess {
public:
    RS485Transceiver() : PostProcess("RS485-RX") {}

    void setup() override {
        startTask(1, 1); // check every 1ms
        rxBuffer.reserve(128);
        Serial.println("Init RS485");
    }

protected:
    void runOnce() override {
        // Always keep newest telemetry cached (no sending here)
        snapshot.ingestFromBus(32);

        if (replying) return;

        HardwareSerial* port = RS485comm::serialPort;
        if (!port) return;

        // Correct non-blocking RX loop
        while (!replying && port->available() > 0) {
            int v = port->read();
            if (v < 0) break;
            processIncoming((char)v);
        }
    }

private:
    TelemetrySnapshot snapshot;

    String rxBuffer;
    bool inPacket = false;
    volatile bool replying = false;

    // --- Your existing formatter stays here (or move it too if you want) ---
    const char* tagToStr(TelemetryTag t) {
        switch (t) {
            case TelemetryTag::CSA: return "CSA";
            case TelemetryTag::CSB: return "CSB";
            case TelemetryTag::OPT1: return "OPT1";
            case TelemetryTag::OPT2: return "OPT2";
            default: return "UNK";
        }
    }

    void sendTelemetry(const TelemetryPacket& p) {
        char line[96];
        snprintf(line, sizeof(line),
                 "#<ACK><%s>(%+ld, %+ld, %+ld)<$>\n",
                 tagToStr(p.tag),
                 (long)p.a, (long)p.b, (long)p.c);

        RS485comm::sendPacket(line);
        Serial.print(line);
    }

    // Static shim so TelemetrySnapshot can call member sendTelemetry
    static void sendTelemetryThunk(const TelemetryPacket& p) {
        instance->sendTelemetry(p);
    }

    // Store instance pointer for thunk use (single instance case)
    static RS485Transceiver* instance;

    void processIncoming(char c) {
        if (!inPacket) {
            if (c == '#') {
                inPacket = true;
                rxBuffer = "";
            }
            return;
        }

        if (c == '\n') {
            handlePacket(rxBuffer);
            inPacket = false;
            return;
        }

        if (c == '\r') return;
        if ((uint8_t)c < 32 || (uint8_t)c > 126) return;

        // Basic max length guard
        if (rxBuffer.length() < 120) rxBuffer += c;
        else { inPacket = false; rxBuffer = ""; }
    }

    void handlePacket(String cmdRaw) {
        cmdRaw.trim();
        if (cmdRaw.length() == 0) return;

        String cmd = cmdRaw;
        cmd.toUpperCase();

        // Support: "DATA" or "DATA CSA"
        if (cmd == "DATA" || cmd.startsWith("DATA ")) {
            replying = true;
            //RS485comm::enableTX();

            // ensure thunk has the right instance
            instance = this;

            if (cmd == "DATA") {
                snapshot.sendAll(&RS485Transceiver::sendTelemetryThunk);
            } else {
                // Parse tag after "DATA "
                String arg = cmdRaw.substring(4); // keep original spacing, case irrelevant
                TelemetryTag t;
                if (TelemetrySnapshot::parseTag(arg, t)) {
                    snapshot.sendOne(t, &RS485Transceiver::sendTelemetryThunk);
                } else {
                    RS485comm::sendPacket("#<ACK><DATA>(BADTAG)<$>");
                }
            }

            //RS485comm::enableRX();
            
            return;
        }

        // Unknown command
        RS485comm::sendPacket("#<ACK><CMD>(UNKNOWN)<$>");
        replying = false;
    }
};

// define static
RS485Transceiver* RS485Transceiver::instance = nullptr;

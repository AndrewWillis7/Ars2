#pragma once
#include <Arduino.h>
#include <RS485comm.h>
#include <string.h>
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

    int setupArray[8] = {};

protected:
    void runOnce() override {
        // Always keep newest telemetry cached (no sending here)
        snapshot.ingestFromBus(32);

        if (replying) return;

        HardwareSerial* port = RS485comm::serialPort;
        if (!port) return;

        // Correct non-blocking RX loop
        while (!replying) {
            int v = port->read();
            //Serial.println(v);
            if (v < 0) break;
            processIncoming((char)v);
        }
    }

private:
    TelemetrySnapshot snapshot;

    String rxBuffer;
    bool inPacket = false;
    volatile bool replying = false;

    void sendTelemetry(const TelemetryPacket& p) {
        char line[96];
        snprintf(line, sizeof(line),
                 "%s(%+ld, %+ld, %+ld)<$>",
                 p.name,
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
        //i
        RS485comm::enableRX();

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
        //cmdRaw.trim();
        if (cmdRaw.length() == 0) return;

        String cmd = cmdRaw;
        cmd.toUpperCase();

        Serial.print("Recieved: ");
        Serial.println(cmdRaw);

        if (cmd.indexOf("DATA") > -1) {
            //replying = true;
            //RS485comm::enableTX();

            // ensure thunk has the right instance
            instance = this;

            RS485comm::sendPacket("<ACK><DATA>");
            snapshot.sendAll(&RS485Transceiver::sendTelemetryThunk);
            RS485comm::sendPacket("<EOL>");

            /*
            if (cmd == "DATA") {
                snapshot.sendAll(&RS485Transceiver::sendTelemetryThunk);
                RS485comm::sendPacket("<EOL>");
            } else {
                // Parse tag after "DATA "
                String arg = cmdRaw.substring(4); // keep original spacing, case irrelevant
                TelemetryTag t;
                if (TelemetrySnapshot::parseTag(arg, t)) {
                    snapshot.sendOne(t, &RS485Transceiver::sendTelemetryThunk);
                    RS485comm::sendPacket("<EOL>");
                } else {
                    RS485comm::sendPacket("<ACK><DATA>(BADTAG)<EOL>");
                }
            }
            */

            //RS485comm::enableRX();
            
            return;
        }

        if (cmd.indexOf("<OFFS>") >= 0) {

            int start = cmd.indexOf("OPTL(");
            int end   = cmd.indexOf(")", start);

            if (start < 0 || end < 0) {
                RS485comm::sendPacket("<ACK><OFFS>(BADFORMAT)<EOL>");
                return;
            }

            // Clear and resize to exactly 6 elements
            globals::offsets.clear();
            globals::offsets.resize(6);

            auto parseTriple = [&](int baseIndex, const char* tag) -> bool {

                int s = cmd.indexOf(tag);
                if (s < 0) return false;

                int open  = cmd.indexOf('(', s);
                int close = cmd.indexOf(')', open);
                if (open < 0 || close < 0) return false;

                String triple = cmd.substring(open + 1, close);

                int c1 = triple.indexOf(',');
                int c2 = triple.indexOf(',', c1 + 1);

                if (c1 < 0 || c2 < 0) return false;

                String xStr = triple.substring(0, c1);
                String yStr = triple.substring(c1 + 1, c2);
                String hStr = triple.substring(c2 + 1);

                xStr.trim();
                yStr.trim();
                hStr.trim();

                globals::offsets[baseIndex + 0] = xStr.toFloat();
                globals::offsets[baseIndex + 1] = yStr.toFloat();
                globals::offsets[baseIndex + 2] = hStr.toFloat();

                return true;
            };

            bool okL = parseTriple(0, "OPTL");
            bool okR = parseTriple(3, "OPTR");

            if (!okL || !okR) {
                RS485comm::sendPacket("<ACK><OFFS>(BADARGS)<EOL>");
                return;
            }

            RS485comm::sendPacket("<ACK><OFFS>(OK)<EOL>");
            return;
        }

        if (cmd.indexOf("HRST") >= 0) {
            ESP.restart();
        }

        if (cmd.indexOf("SRST") >= 0) {
            if (globals::state == globals::SystemState::RUNNING) ESP.restart();
        }


        if (cmd.indexOf("INIT") >= 0) {

            if (globals::state == globals::SystemState::RUNNING) {
                RS485comm::sendPacket("<ACK><INIT>(ALREADY_CONFIGURED)<EOL>");
                return;
            }

            int open = cmd.indexOf('(');
            int close = cmd.lastIndexOf(')');

            if (open < 0 || close < 0 || close <= open) {
                RS485comm::sendPacket("<ACK><INIT>(BADFORMAT)<EOL>");
                return;
            }

            // Keep parentheses so the first tuple is intact
            String args = cmd.substring(open, close + 1);
            args.trim();

            size_t added = 0, bad = 0;

            int i = 0;
            while (true) {
                int tupleStart = args.indexOf('(', i);
                if (tupleStart < 0) break;

                int tupleEnd = args.indexOf(')', tupleStart + 1);
                if (tupleEnd < 0) { bad++; break; }

                String tuple = args.substring(tupleStart + 1, tupleEnd); // inside (...)
                tuple.trim();

                int c1 = tuple.indexOf(',');
                int c2 = tuple.indexOf(',', c1 + 1);

                if (c1 < 0 || c2 < 0) {
                    bad++;
                } else {
                    globals::SensorConfig cfg;

                    cfg.name = tuple.substring(0, c1); cfg.name.trim();
                    cfg.type = tuple.substring(c1 + 1, c2); cfg.type.trim(); cfg.type.toUpperCase();
                    String portStr = tuple.substring(c2 + 1); portStr.trim();

                    cfg.port = (uint8_t)portStr.toInt();

                    globals::sensors.push_back(cfg);
                    added++;
                }

                i = tupleEnd + 1; // continue after this ')'
            }

            char resp[80];
            snprintf(resp, sizeof(resp), "<ACK><INIT>(ADDED=%u,BAD=%u)<EOL>",
                    (unsigned)added, (unsigned)bad);
            RS485comm::sendPacket(resp);

            globals::state = globals::SystemState::RUNNING;

            return;
        }

        // Ping Pong
        if (cmd.indexOf("PING") > -1) {
            RS485comm::sendPacket("<ACK><UNKO>(PONG-PONG)<EOL>");
            return;
        } 

        // Unknown command
        RS485comm::sendPacket("<ACK><ERRR>(UNKNOWN)<EOL>");
        //replying = false;
    }
};

// define static
RS485Transceiver* RS485Transceiver::instance = nullptr;

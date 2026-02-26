#pragma once
#include <Arduino.h>
#include <RS485comm.h>
#include <string.h>
#include "../lib/post_process.h"

#include <TelemetryBus.h>
#include "TelemetrySnapshot.h"
#include "../globals.h"

// ESPNOW raw forwarding helper (header-only)
#include "../lib/wifi_forward.h"

// Updated RS485Transceiver:
// - Normal RS485 parsing/command handling still works
// - New command: WIFI / WIFI OFF toggles globals::wifi_enabled
// - When wifi_enabled is true, EVERY raw RX byte read from RS485 is forwarded over ESP-NOW
//   (buffered and flushed on '\n' or when buffer fills)

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
        // Cache newest telemetry (unchanged behavior)
        snapshot.ingestFromBus(32);

        HardwareSerial* port = RS485comm::serialPort;
        if (!port) return;

        // Bring ESPNOW up/down based on flag
        if (globals::wifi_enabled) nowEnsureUp();
        else nowEnsureDown();

        // Non-blocking RX loop
        while (true) {
            int v = port->read();
            if (v < 0) break;

            char c = (char)v;

            // Existing command parser (expects # ... \n)
            processIncoming(c);

            // ALSO forward raw bytes over ESP-NOW when enabled
            if (globals::wifi_enabled && wifi_forward::isUp()) {
                //Serial.println("Pushing Bytes");
                nowPushByte((uint8_t)v);
            }
        }
    }

private:
    // ----- Telemetry snapshot -----
    TelemetrySnapshot snapshot;

    // ----- RS485 command parsing -----
    String rxBuffer;
    bool inPacket = false;

    // ----- ESPNOW raw forwarding -----
    static constexpr size_t NOW_BUF_SZ = 200;
    uint8_t nowBuf[NOW_BUF_SZ];
    size_t nowLen = 0;

    bool nowStarted = false;

    // TODO: SET THIS to your RECEIVER ESP32 MAC address
    static const uint8_t NOW_PEER[6];

    void nowEnsureUp() {
        if (nowStarted && wifi_forward::isUp()) return;
        nowStarted = wifi_forward::begin(NOW_PEER);
    }

    void nowEnsureDown() {
        if (!nowStarted) return;
        wifi_forward::end();
        nowStarted = false;
        nowLen = 0;
    }

    void nowPushByte(uint8_t b) {
        nowBuf[nowLen++] = b;

        // Flush on newline or full buffer
        if (b == '\n' || nowLen >= NOW_BUF_SZ) {
            wifi_forward::sendBytes(nowBuf, nowLen);
            nowLen = 0;
        }
    }

    // ----- Original TX helper (kept, used by DATA) -----
    void sendTelemetry(const TelemetryPacket& p) {
        char line[96];
        snprintf(line, sizeof(line),
                 "%s(%+ld, %+ld, %+ld)<$>",
                 p.name,
                 (long)p.a, (long)p.b, (long)p.c);

        RS485comm::sendPacket(line);
        Serial.print(line);
    }

    // Static thunk for snapshot.sendAll callback
    static RS485Transceiver* instance;
    static void sendTelemetryThunk(const TelemetryPacket& p) {
        if (instance) instance->sendTelemetry(p);
    }

    // ----- RS485 RX state machine -----
    void processIncoming(char c) {
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

        // Only printable ASCII to avoid garbage in commands
        if ((uint8_t)c < 32 || (uint8_t)c > 126) return;

        // Basic max length guard
        if (rxBuffer.length() < 120) rxBuffer += c;
        else { inPacket = false; rxBuffer = ""; }
    }

    void handlePacket(String cmdRaw) {
        if (cmdRaw.length() == 0) return;

        String cmd = cmdRaw;
        cmd.toUpperCase();

        Serial.print("Recieved: ");
        Serial.println(cmdRaw);

        // ---- WIFI / WIFI OFF ----
        // Only toggles forwarding; no other behavior.
        if (cmd.indexOf("WIFI") >= 0) {
            bool en = true;
            if (cmd.indexOf("OFF") >= 0) en = false;

            globals::wifi_enabled = en;
            RS485comm::sendPacket(en ? "<ACK><WIFI>(ON)<EOL>" : "<ACK><WIFI>(OFF)<EOL>");
            return;
        }

        // ---- DATA ----
        // Respond with newest cached telemetry snapshot (unchanged)
        if (cmd.indexOf("DATA") >= 0) {
            instance = this;
            RS485comm::sendPacket("<ACK><DATA>");
            snapshot.sendAll(&RS485Transceiver::sendTelemetryThunk);
            RS485comm::sendPacket("<EOL>");
            return;
        }

        // ---- OFFS ----
        if (cmd.indexOf("<OFFS>") >= 0) {
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

                xStr.trim(); yStr.trim(); hStr.trim();

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

        // ---- resets ----
        if (cmd.indexOf("HRST") >= 0) {
            ESP.restart();
        }

        if (cmd.indexOf("SRST") >= 0) {
            if (globals::state == globals::SystemState::RUNNING) ESP.restart();
        }

        // ---- INIT ----
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

            String args = cmd.substring(open, close + 1);
            args.trim();

            size_t added = 0, bad = 0;

            int i = 0;
            while (true) {
                int tupleStart = args.indexOf('(', i);
                if (tupleStart < 0) break;

                int tupleEnd = args.indexOf(')', tupleStart + 1);
                if (tupleEnd < 0) { bad++; break; }

                String tuple = args.substring(tupleStart + 1, tupleEnd);
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

                i = tupleEnd + 1;
            }

            char resp[80];
            snprintf(resp, sizeof(resp), "<ACK><INIT>(ADDED=%u,BAD=%u)<EOL>",
                     (unsigned)added, (unsigned)bad);
            RS485comm::sendPacket(resp);

            globals::state = globals::SystemState::RUNNING;
            return;
        }

        // ---- WIFI Send ----
        if (cmd.indexOf("WIDA") >= 0) {
            //Serial.println("Streamed");
            return;
        }

        // ---- Ping/Pong ----
        if (cmd.indexOf("PING") >= 0) {
            RS485comm::sendPacket("<ACK><UNKO>(PONG-PONG)<EOL>");
            return;
        }

        RS485comm::sendPacket("<ACK><ERRR>(UNKNOWN)<EOL>");
    }
};

// static init
RS485Transceiver* RS485Transceiver::instance = nullptr;
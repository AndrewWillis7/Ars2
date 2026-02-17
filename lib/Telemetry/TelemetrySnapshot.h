#pragma once
#include <Arduino.h>
#include <TelemetryBus.h>

class TelemetrySnapshot {
public:
    // Drain telemetry queue and update the latest cache memory
    void ingestFromBus(uint32_t maxDrain = 32) {
        TelemetryPacket p;
        for (uint32_t i = 0; i < maxDrain; i++) {
            if (!TelemetryBus::receive(p, 0)) break;
            updateCache(p);
        }
    }

    // Send a snapshot when requested
    void sendAll(void (*sendFn)(const TelemetryPacket&)) const {
        if (!sendFn) return;
        if (hasCS1)  sendFn(latestCS1);
        if (hasCS2)  sendFn(latestCS2);
        if (hasOPT1) sendFn(latestOPT1);
        if (hasOPT2) sendFn(latestOPT2);
    }

    bool sendOne(TelemetryTag tag, void (*sendFn)(const TelemetryPacket&)) {
        if (!sendFn) return false;
        switch (tag) {
            case TelemetryTag::CSA:  if (hasCS1)  { sendFn(latestCS1);  return true; } break;
            case TelemetryTag::CSB:  if (hasCS2)  { sendFn(latestCS2);  return true; } break;
            case TelemetryTag::OPT1: if (hasOPT1) { sendFn(latestOPT1); return true; } break;
            case TelemetryTag::OPT2: if (hasOPT2) { sendFn(latestOPT2); return true; } break;
            default: break;
        }
        return false;
    }

    static bool parseTag(const String& sRaw, TelemetryTag& out) {
        String s = sRaw;
        s.trim();
        s.toUpperCase();
        if (s == "CSA")  { out = TelemetryTag::CSA;  return true; }
        if (s == "CSB")  { out = TelemetryTag::CSB;  return true; }
        if (s == "OPT1") { out = TelemetryTag::OPT1; return true; }
        if (s == "OPT2") { out = TelemetryTag::OPT2; return true; }
        return false;
    }

private:
    TelemetryPacket latestCS1{}, latestCS2{}, latestOPT1{}, latestOPT2{};
    bool hasCS1 = false, hasCS2 = false, hasOPT1 = false, hasOPT2 = false;

    void updateCache(const TelemetryPacket& p) {
        switch (p.tag) {
            case TelemetryTag::CSA:  latestCS1  = p; hasCS1  = true; break;
            case TelemetryTag::CSB:  latestCS2  = p; hasCS2  = true; break;
            case TelemetryTag::OPT1: latestOPT1 = p; hasOPT1 = true; break;
            case TelemetryTag::OPT2: latestOPT2 = p; hasOPT2 = true; break;
            default: break;
        }
    }
};
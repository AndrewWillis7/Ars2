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
        if (hasOPTL) sendFn(latestOPTL);
        if (hasOPTR) sendFn(latestOPTR);
    }

    bool sendOne(TelemetryTag tag, void (*sendFn)(const TelemetryPacket&)) {
        if (!sendFn) return false;
        switch (tag) {
            case TelemetryTag::CSA:  if (hasCS1)  { sendFn(latestCS1);  return true; } break;
            case TelemetryTag::CSB:  if (hasCS2)  { sendFn(latestCS2);  return true; } break;
            case TelemetryTag::OPTL: if (hasOPTL) { sendFn(latestOPTL); return true; } break;
            case TelemetryTag::OPTR: if (hasOPTR) { sendFn(latestOPTR); return true; } break;
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
        if (s == "OPTL") { out = TelemetryTag::OPTL; return true; }
        if (s == "OPTR") { out = TelemetryTag::OPTR; return true; }
        return false;
    }

private:
    TelemetryPacket latestCS1{}, latestCS2{}, latestOPTL{}, latestOPTR{};
    bool hasCS1 = false, hasCS2 = false, hasOPTL = false, hasOPTR = false;

    void updateCache(const TelemetryPacket& p) {
        switch (p.tag) {
            case TelemetryTag::CSA:  latestCS1  = p; hasCS1  = true; break;
            case TelemetryTag::CSB:  latestCS2  = p; hasCS2  = true; break;
            case TelemetryTag::OPTL: latestOPTL = p; hasOPTL = true; break;
            case TelemetryTag::OPTR: latestOPTR = p; hasOPTR = true; break;
            default: break;
        }
    }
};
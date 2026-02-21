#pragma once
#include <Arduino.h>
#include <TelemetryBus.h>

class TelemetrySnapshot {
public:
    // Tune this to your max sensor count (you reserve 16 elsewhere)
    static constexpr size_t MAX_SENSORS = 16;

    void ingestFromBus(uint32_t maxDrain = 32) {
        TelemetryPacket p;
        for (uint32_t i = 0; i < maxDrain; i++) {
            if (!TelemetryBus::receive(p, 0)) break;
            updateCache(p);
        }
    }

    void sendAll(void (*sendFn)(const TelemetryPacket&)) const {
        if (!sendFn) return;

        for (size_t i = 0; i < _count; i++) {
            if (_entries[i].valid) sendFn(_entries[i].pkt);
        }
    }

    // Optional: send one packet by name (for DATA(Color1) style requests)
    bool sendOneByName(const String& nameRaw, void (*sendFn)(const TelemetryPacket&)) const {
        if (!sendFn) return false;

        String name = nameRaw;
        name.trim();
        if (name.length() == 0) return false;

        for (size_t i = 0; i < _count; i++) {
            if (!_entries[i].valid) continue;
            if (_entries[i].pkt.name && name.equalsIgnoreCase(_entries[i].pkt.name)) {
                sendFn(_entries[i].pkt);
                return true;
            }
        }
        return false;
    }

private:
    struct Entry {
        TelemetryPacket pkt{};
        bool valid = false;
    };

    Entry _entries[MAX_SENSORS]{};
    size_t _count = 0;

    void updateCache(const TelemetryPacket& p) {
        if (!p.name) return;

        // 1) Update existing entry if name matches
        for (size_t i = 0; i < _count; i++) {
            if (_entries[i].valid && _entries[i].pkt.name && strcmp(_entries[i].pkt.name, p.name) == 0) {
                _entries[i].pkt = p;
                _entries[i].valid = true;
                return;
            }
        }

        // 2) Otherwise add new entry (if space)
        if (_count < MAX_SENSORS) {
            _entries[_count].pkt = p;
            _entries[_count].valid = true;
            _count++;
        }

        //TODO: 3) If full, you can either drop or overwrite (currently drop).
    }
};
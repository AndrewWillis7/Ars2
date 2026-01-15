#pragma once
#include <Arduino.h>
#include <vector>
#include <stdint.h>
// TODO:
/*
    The Goal is to get the stack of sensors from the scheduler and instance the sensors using the stack data

    * Might need to provide the method for the I2C scan to contact the scheduler here as well
*/

class SensorBase; //  forward Declaration

// Core structures that provide CONTEXT to the arsenal about what is connected
struct SensorDescriptor {
    const char* typeName;

    // presence check for I2C addresses
    const uint8_t* candidateAddress;
    uint8_t numAddress;

    // confirm type identity
    bool (*probe)(uint8_t muxChannel);

    // Build the object
    SensorBase* (*create)(const char* instanceName, uint8_t muxChannel);
};

struct DiscoveredSensor {
    const SensorDescriptor* desc;
    uint8_t muxChannel;
    const char* instanceName;
};

#include "scheduler.h"

class Allocator {
public:
    // Singleton accessor
    static Allocator& instance() {
        static Allocator inst;
        return inst;
    }

    void begin(const SensorDescriptor* table, size_t tableCount,
        const uint8_t* muxChannels, size_t muxCount)
    {
        _table = table;
        _tableCount = tableCount;
        _muxChannels = muxChannels;
        _muxCount = muxCount;
    }

    size_t discoverAndAllocate() {
        _found.clear(); // reset

        // Phase 1 & 2 are combined because we are smart and can handle it
        for (size_t i = 0; i < _muxCount; i++) {
            uint8_t ch = _muxChannels[i];

            for (size_t v = 0; v < _tableCount; v++) {
                const SensorDescriptor& d = _table[v];

                if (!d.probe) continue;

                if (d.probe(ch)) {
                    // Create instance name like OTOS_1, Color_2, etc
                    const char* name = makeInstanceName(d.typeName);
                    
                    // ask chat about this one its wacky
                    SensorBase* obj = d.create ? d.create(name, ch) : nullptr;
                    if (obj) {
                        // This is where we could call object specific functions if needed, 
                        // IE: register it to Schedular (which we already do in the sensor construction)
                        _found.push_back({&d, ch, name});
                    }
                }
            }
        }

        return _found.size();
    }

    const std::vector<DiscoveredSensor>& found() const { return _found; }

private:
    Allocator() = default;

    const SensorDescriptor* _table = nullptr;
    size_t _tableCount = 0;
    const uint8_t* _muxChannels = nullptr;
    size_t _muxCount = 0;

    std::vector<DiscoveredSensor> _found;

    const char* makeInstanceName(const char* base) {
        static char buf[24];
        static uint16_t idx = 0;
        snprintf(buf, sizeof(buf), "%s_%u", base, (unsigned)idx++);
        return buf;
        // NOTE: I dont think this is a good solution, because in the long term we want per-instance storage
    }
};
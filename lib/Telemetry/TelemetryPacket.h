#pragma once
#include <stdint.h>

enum class TelemetryTag: uint8_t {
    CSA,
    CSB,
    OPT1,
    OPT2
};

struct TelemetryPacket
{
    TelemetryTag tag;
    int32_t a;
    int32_t b;
    int32_t c;
    uint32_t ms; // timestamp
};

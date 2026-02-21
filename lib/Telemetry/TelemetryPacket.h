#pragma once
#include <stdint.h>

enum class TelemetryTag: uint8_t {
    CSA,
    CSB,
    OPTL,
    OPTR,
    NONE
};

struct TelemetryPacket
{
    TelemetryTag tag;
    const char* name;
    int32_t a;
    int32_t b;
    int32_t c;
    uint32_t ms; // timestamp
};

#pragma once
#include <stdint.h>

#pragma once
#include <stdint.h>

struct TelemetryPacket
{
    const char* name;   // sensor instance name from INIT, e.g. "CS1"
    int32_t a;
    int32_t b;
    int32_t c;
    uint32_t ms;        // timestamp
};
#pragma once

#include <stdint.h>

struct TelemetryPacket {
    const char* name;
    int32_t a;
    int32_t b;
    int32_t c;
    uint32_t ms;
};

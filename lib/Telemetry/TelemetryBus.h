#pragma once
#include "TelemetryPacket.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace TelemetryBus {
    extern QueueHandle_t q;

    inline void begin(size_t depth = 32) {
        if (!q) q = xQueueCreate(depth, sizeof(TelemetryPacket));
    }

    inline bool publish(const TelemetryPacket& p) {
        return q && xQueueSend(q, &p, 0) == pdTRUE;
    }

    inline bool receive(TelemetryPacket& out, TickType_t wait = 0) {
        return q && xQueueReceive(q, &out, wait) == pdTRUE;
    }
}
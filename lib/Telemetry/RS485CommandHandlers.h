#pragma once

#include <Arduino.h>
#include <TelemetrySnapshot.h>

namespace RS485CommandHandlers {

using TelemetrySender = TelemetrySnapshot::SendFn;

bool dispatch(
    const String& commandRaw,
    TelemetrySnapshot& snapshot,
    TelemetrySender sender,
    void* senderContext);

}  // namespace RS485CommandHandlers

#pragma once

#include <Arduino.h>
#include "../post_process.h"

#include "TelemetrySnapshot.h"

class RS485Transceiver : public PostProcess {
public:
    RS485Transceiver();

    void setup() override;

protected:
    void runOnce() override;

private:
    static void sendTelemetryThunk(const TelemetryPacket& packet, void* context);

    void processIncoming(char c);
    void handlePacket(const String& commandRaw);
    void sendTelemetry(const TelemetryPacket& packet);

    TelemetrySnapshot snapshot_;
    String rxBuffer_;
    bool inPacket_ = false;
};

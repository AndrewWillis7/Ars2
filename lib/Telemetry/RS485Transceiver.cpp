#include "RS485Transceiver.h"

#include "RS485CommandHandlers.h"

#include <RS485comm.h>

RS485Transceiver::RS485Transceiver()
    : PostProcess("RS485-RX") {}

void RS485Transceiver::setup() {
    rxBuffer_.reserve(128);
    startTask(1, 1);
    Serial.println("Init RS485");
}

void RS485Transceiver::runOnce() {
    snapshot_.ingestFromBus(32);

    HardwareSerial* port = RS485comm::serialPort;
    if (!port) {
        return;
    }

    while (true) {
        const int value = port->read();
        if (value < 0) {
            break;
        }

        processIncoming(static_cast<char>(value));
    }
}

void RS485Transceiver::sendTelemetryThunk(const TelemetryPacket& packet, void* context) {
    if (!context) {
        return;
    }

    static_cast<RS485Transceiver*>(context)->sendTelemetry(packet);
}

void RS485Transceiver::processIncoming(char c) {
    if (!inPacket_) {
        if (c == '#') {
            inPacket_ = true;
            rxBuffer_ = "";
        }
        return;
    }

    if (c == '\n') {
        handlePacket(rxBuffer_);
        inPacket_ = false;
        return;
    }

    if (c == '\r') {
        return;
    }

    if (static_cast<uint8_t>(c) < 32 || static_cast<uint8_t>(c) > 126) {
        return;
    }

    if (rxBuffer_.length() < 120) {
        rxBuffer_ += c;
    } else {
        inPacket_ = false;
        rxBuffer_ = "";
    }
}

void RS485Transceiver::handlePacket(const String& commandRaw) {
    Serial.print("Received: ");
    Serial.println(commandRaw);

    RS485CommandHandlers::dispatch(
        commandRaw,
        snapshot_,
        &RS485Transceiver::sendTelemetryThunk,
        this);
}

void RS485Transceiver::sendTelemetry(const TelemetryPacket& packet) {
    char line[96];
    snprintf(
        line,
        sizeof(line),
        "%s(%+ld, %+ld, %+ld)<$>",
        packet.name,
        static_cast<long>(packet.a),
        static_cast<long>(packet.b),
        static_cast<long>(packet.c));

    RS485comm::sendPacket(line);
    Serial.print(line);
}

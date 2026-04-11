#include "RS485CommandHandlers.h"

#include <RS485comm.h>
#include <RuntimeConfig.h>

#include <array>
#include <vector>

namespace {

bool extractTuples(const String& args, std::vector<String>& tuples, size_t& badCount) {
    int cursor = 0;

    while (true) {
        const int tupleStart = args.indexOf('(', cursor);
        if (tupleStart < 0) {
            return true;
        }

        const int tupleEnd = args.indexOf(')', tupleStart + 1);
        if (tupleEnd < 0) {
            ++badCount;
            return false;
        }

        String tuple = args.substring(tupleStart + 1, tupleEnd);
        tuple.trim();
        tuples.push_back(tuple);
        cursor = tupleEnd + 1;
    }
}

String commandPayload(const String& normalizedCommand) {
    const int open = normalizedCommand.indexOf('(');
    const int close = normalizedCommand.lastIndexOf(')');

    if (open < 0 || close < 0 || close <= open) {
        return String();
    }

    String args = normalizedCommand.substring(open, close + 1);
    args.trim();
    return args;
}

bool parseFloatTriple(
    const String& command,
    const char* tag,
    size_t baseIndex,
    std::array<float, 6>& offsets) {
    const int tagIndex = command.indexOf(tag);
    if (tagIndex < 0) {
        return false;
    }

    const int open = command.indexOf('(', tagIndex);
    const int close = command.indexOf(')', open);
    if (open < 0 || close < 0) {
        return false;
    }

    const String triple = command.substring(open + 1, close);
    const int c1 = triple.indexOf(',');
    const int c2 = triple.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) {
        return false;
    }

    String xStr = triple.substring(0, c1);
    String yStr = triple.substring(c1 + 1, c2);
    String hStr = triple.substring(c2 + 1);

    xStr.trim();
    yStr.trim();
    hStr.trim();

    offsets[baseIndex + 0] = xStr.toFloat();
    offsets[baseIndex + 1] = yStr.toFloat();
    offsets[baseIndex + 2] = hStr.toFloat();
    return true;
}

bool handleDataCommand(
    const String& normalizedCommand,
    TelemetrySnapshot& snapshot,
    RS485CommandHandlers::TelemetrySender sender,
    void* senderContext) {
    if (normalizedCommand.indexOf("DATA") < 0) {
        return false;
    }

    RS485comm::sendPacket("<ACK><DATA>");
    snapshot.sendAll(sender, senderContext);
    RS485comm::sendPacket("<EOL>");
    return true;
}

bool handleOffsetsCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("<OFFS>") < 0) {
        return false;
    }

    if (normalizedCommand.indexOf("OPTL(") < 0 || normalizedCommand.indexOf("OPTR(") < 0) {
        RS485comm::sendPacket("<ACK><OFFS>(BADFORMAT)<EOL>");
        return true;
    }

    std::array<float, 6> offsets = RuntimeConfig::offsets();
    const bool okLeft = parseFloatTriple(normalizedCommand, "OPTL", 0, offsets);
    const bool okRight = parseFloatTriple(normalizedCommand, "OPTR", 3, offsets);

    if (!okLeft || !okRight) {
        RS485comm::sendPacket("<ACK><OFFS>(BADARGS)<EOL>");
        return true;
    }

    RuntimeConfig::setOffsets(offsets);
    RS485comm::sendPacket("<ACK><OFFS>(OK)<EOL>");
    return true;
}

bool handleResetCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("HRST") >= 0) {
        ESP.restart();
        return true;
    }

    if (normalizedCommand.indexOf("SRST") >= 0) {
        if (RuntimeConfig::isRunning()) {
            ESP.restart();
        }
        return true;
    }

    return false;
}

bool handleInitCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("INIT") < 0) {
        return false;
    }

    if (RuntimeConfig::isRunning()) {
        RS485comm::sendPacket("<ACK><INIT>(ALREADY_CONFIGURED)<EOL>");
        return true;
    }

    const String args = commandPayload(normalizedCommand);
    if (args.length() == 0) {
        RS485comm::sendPacket("<ACK><INIT>(BADFORMAT)<EOL>");
        return true;
    }

    std::vector<String> tuples;
    size_t added = 0;
    size_t bad = 0;
    extractTuples(args, tuples, bad);

    std::vector<RuntimeConfig::SensorConfig> configs;
    configs.reserve(tuples.size());

    for (const auto& tuple : tuples) {
        const int c1 = tuple.indexOf(',');
        const int c2 = tuple.indexOf(',', c1 + 1);

        if (c1 < 0 || c2 < 0) {
            ++bad;
            continue;
        }

        RuntimeConfig::SensorConfig cfg;
        cfg.name = tuple.substring(0, c1);
        cfg.name.trim();
        cfg.type = tuple.substring(c1 + 1, c2);
        cfg.type.trim();
        cfg.type.toUpperCase();

        String portStr = tuple.substring(c2 + 1);
        portStr.trim();
        cfg.port = static_cast<uint8_t>(portStr.toInt());

        configs.push_back(cfg);
        ++added;
    }

    RuntimeConfig::setSensorConfigs(configs);
    RuntimeConfig::setState(RuntimeConfig::SystemState::RUNNING);

    char response[80];
    snprintf(
        response,
        sizeof(response),
        "<ACK><INIT>(ADDED=%u,BAD=%u)<EOL>",
        static_cast<unsigned>(added),
        static_cast<unsigned>(bad));
    RS485comm::sendPacket(response);
    return true;
}

bool handleLightInitCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("LINT") < 0) {
        return false;
    }

    if (RuntimeConfig::hasLightConfig()) {
        RS485comm::sendPacket("<ACK><LINT>(ALREADY_CONFIGURED)<EOL>");
        return true;
    }

    const String args = commandPayload(normalizedCommand);
    if (args.length() == 0) {
        RS485comm::sendPacket("<ACK><LINT>(BADFORMAT)<EOL>");
        return true;
    }

    std::vector<String> tuples;
    size_t added = 0;
    size_t bad = 0;
    extractTuples(args, tuples, bad);

    std::vector<RuntimeConfig::LightConfig> configs;
    configs.reserve(tuples.size());

    for (const auto& tuple : tuples) {
        const int c1 = tuple.indexOf(',');
        const int c2 = tuple.indexOf(',', c1 + 1);

        if (c1 < 0 || c2 < 0) {
            ++bad;
            continue;
        }

        RuntimeConfig::LightConfig cfg;
        cfg.name = tuple.substring(0, c1);
        cfg.name.trim();

        String gpioStr = tuple.substring(c1 + 1, c2);
        gpioStr.trim();
        String countStr = tuple.substring(c2 + 1);
        countStr.trim();

        cfg.gpio = static_cast<uint8_t>(gpioStr.toInt());
        cfg.count = static_cast<uint16_t>(countStr.toInt());

        if (cfg.name.length() == 0 || cfg.count == 0) {
            ++bad;
            continue;
        }

        configs.push_back(cfg);
        ++added;
    }

    if (!configs.empty()) {
        RuntimeConfig::setLightConfigs(configs);
    }

    char response[80];
    snprintf(
        response,
        sizeof(response),
        "<ACK><LINT>(ADDED=%u,BAD=%u)<EOL>",
        static_cast<unsigned>(added),
        static_cast<unsigned>(bad));
    RS485comm::sendPacket(response);
    return true;
}

bool handleLightSetCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("LSET") < 0) {
        return false;
    }

    if (!RuntimeConfig::hasLightConfig()) {
        RS485comm::sendPacket("<ACK><LSET>(NO_LIGHTS)<EOL>");
        return true;
    }

    const String args = commandPayload(normalizedCommand);
    if (args.length() == 0) {
        RS485comm::sendPacket("<ACK><LSET>(BADFORMAT)<EOL>");
        return true;
    }

    std::vector<String> tuples;
    size_t updated = 0;
    size_t missing = 0;
    size_t bad = 0;
    extractTuples(args, tuples, bad);

    for (const auto& tuple : tuples) {
        const int c1 = tuple.indexOf(',');
        const int c2 = tuple.indexOf(',', c1 + 1);
        const int c3 = tuple.indexOf(',', c2 + 1);
        const int c4 = tuple.indexOf(',', c3 + 1);
        const int c5 = tuple.indexOf(',', c4 + 1);
        const int c6 = tuple.indexOf(',', c5 + 1);

        if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 || c5 < 0 || c6 < 0) {
            ++bad;
            continue;
        }

        RuntimeConfig::LightState state;
        state.name = tuple.substring(0, c1);
        state.name.trim();

        String rStr = tuple.substring(c1 + 1, c2);
        String gStr = tuple.substring(c2 + 1, c3);
        String bStr = tuple.substring(c3 + 1, c4);
        String patternStr = tuple.substring(c4 + 1, c5);
        String brightnessStr = tuple.substring(c5 + 1, c6);
        String periodStr = tuple.substring(c6 + 1);

        rStr.trim();
        gStr.trim();
        bStr.trim();
        patternStr.trim();
        brightnessStr.trim();
        periodStr.trim();

        state.r = static_cast<uint8_t>(constrain(rStr.toInt(), 0, 255));
        state.g = static_cast<uint8_t>(constrain(gStr.toInt(), 0, 255));
        state.b = static_cast<uint8_t>(constrain(bStr.toInt(), 0, 255));
        state.pattern = RuntimeConfig::parseLightPattern(patternStr);
        state.brightness = static_cast<uint8_t>(constrain(brightnessStr.toInt(), 0, 255));
        state.periodMs = static_cast<uint16_t>(max(1L, static_cast<long>(periodStr.toInt())));

        if (RuntimeConfig::updateLightState(state.name, state)) {
            ++updated;
        } else {
            ++missing;
        }
    }

    char response[96];
    snprintf(
        response,
        sizeof(response),
        "<ACK><LSET>(UPDATED=%u,MISSING=%u,BAD=%u)<EOL>",
        static_cast<unsigned>(updated),
        static_cast<unsigned>(missing),
        static_cast<unsigned>(bad));
    RS485comm::sendPacket(response);
    return true;
}

bool handlePingCommand(const String& normalizedCommand) {
    if (normalizedCommand.indexOf("PING") < 0) {
        return false;
    }

    RS485comm::sendPacket("<ACK><UNKO>(PONG-PONG)<EOL>");
    return true;
}

}  // namespace

namespace RS485CommandHandlers {

bool dispatch(
    const String& commandRaw,
    TelemetrySnapshot& snapshot,
    TelemetrySender sender,
    void* senderContext) {
    if (commandRaw.length() == 0) {
        return true;
    }

    String normalized = commandRaw;
    normalized.trim();
    normalized.toUpperCase();

    if (handleDataCommand(normalized, snapshot, sender, senderContext)) {
        return true;
    }
    if (handleOffsetsCommand(normalized)) {
        return true;
    }
    if (handleResetCommand(normalized)) {
        return true;
    }
    if (handleInitCommand(normalized)) {
        return true;
    }
    if (handleLightInitCommand(normalized)) {
        return true;
    }
    if (handleLightSetCommand(normalized)) {
        return true;
    }
    if (handlePingCommand(normalized)) {
        return true;
    }

    RS485comm::sendPacket("<ACK><ERRR>(UNKNOWN)<EOL>");
    return false;
}

}  // namespace RS485CommandHandlers

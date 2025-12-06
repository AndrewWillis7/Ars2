#pragma once

// -- Color Defs --

// ANSI colors for pretty CLI output
#define C_RESET     "\033[0m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_BOLD      "\033[1m"

// Unicode symbols (safe in VSCode terminal?)
#define SYM_CHECK   "✓"
#define SYM_FAIL    "✗"
#define SYM_WARN    "⚠"
#define SYM_INFO    "ℹ"
#define SYM_ARROW   "➜"
#define SYM_DOT     "•"

// -- Actual Program down here --
#include <Arduino.h>
#include <vector>
#include "sensor_base.h"
#include <I2CUtils.h>

class DebugConsole {
public:
    DebugConsole() {}

    void registerSensor(SensorBase* sensor) {
        sensors.push_back(sensor);
    }

    // Call in loop
    void update();

private:
    String inputBuffer;
    std::vector<SensorBase*> sensors;

    // watch mode
    bool watchMode = false;
    uint32_t watchInterval = 500;
    uint32_t lastWatch = 0;
    String watchCommand = "";

    // Command Handlers
    void handleCommand(const String& cmd);
    void printHelp();
    void printAllSensorStats();
    void printI2CStats();
    void scanI2C();
    void taskStatus();
    void resetStats();
    void muxSwitch(uint8_t ch);
};
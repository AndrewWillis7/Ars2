#include "AppController.h"

#include "SensorFactory.h"

#include <Arduino.h>
#include <I2CUtils.h>
#include <RS485comm.h>
#include <RuntimeConfig.h>
#include <TelemetryBus.h>
#include <hw_config.h>

void AppController::setup() {
    bringUpCore();
    bringUpComms();

    Serial.println("System Online.");

    waitForConfiguration();
    bringUpSensors();

    Serial.println("Sensors Initialized");
    RS485comm::enableRX();
}

void AppController::loop() {
    updateLighting();
    publishHeartbeat();
}

void AppController::bringUpCore() {
    Serial.begin(baudrate);
    delay(50);

    RS485comm::begin(Serial1, baudrate);
    RS485comm::enableRX();

    I2CUtils::begin();
    TelemetryBus::begin(256);
    RuntimeConfig::begin();
    RuntimeConfig::reserveSensors(16);
    RuntimeConfig::reserveLights(8);

    Serial.println("Core Build. Awaiting INIT");
}

void AppController::bringUpComms() {
    rs485Transceiver_.setup();
}

void AppController::waitForConfiguration() {
    while (!RuntimeConfig::isRunning()) {
        Serial.println("Waiting for INIT");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void AppController::bringUpSensors() {
    Serial.println("Configuration Locked. Bring up Sensors");

    const auto configs = RuntimeConfig::sensorConfigs();
    const auto offsets = RuntimeConfig::offsets();

    activeSensors_.clear();
    activeSensors_.reserve(configs.size());

    for (const auto& cfg : configs) {
        std::unique_ptr<SensorBase> sensor = SensorFactory::create(cfg, offsets);
        if (!sensor) {
            Serial.printf("Failed to create sensor: %s\n", cfg.name.c_str());
            continue;
        }

        sensor->setup();
        Serial.printf("Setup sensor: %s on port %u\n", cfg.name.c_str(), cfg.port);
        activeSensors_.push_back(std::move(sensor));
    }

    for (auto& sensor : activeSensors_) {
        sensor->startTask(10, 1);
    }

    Serial.println("All Sensors started!");
}

void AppController::updateLighting() {
    const uint32_t now = millis();
    if (now - lastLightUpdateMs_ < LIGHT_UPDATE_INTERVAL_MS) {
        return;
    }

    lastLightUpdateMs_ = now;

    const uint32_t configVersion = RuntimeConfig::lightConfigVersion();
    if (configVersion != 0 && configVersion != appliedLightConfigVersion_) {
        lightingEngine_.begin(RuntimeConfig::lightConfigs());
        appliedLightConfigVersion_ = configVersion;
    }

    if (lightingEngine_.isInitialized()) {
        lightingEngine_.update(RuntimeConfig::lightStates());
    }
}

void AppController::publishHeartbeat() {
    const uint32_t now = millis();
    if (now - lastHeartbeatMs_ < HEARTBEAT_INTERVAL_MS) {
        return;
    }

    lastHeartbeatMs_ = now;
    Serial.println("[HEARTBEAT] system posted!");
}

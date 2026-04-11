#pragma once

#include <lighting_engine.h>
#include <RS485Transceiver.h>
#include <sensor_base.h>

#include <memory>
#include <vector>

class AppController {
public:
    void setup();
    void loop();

private:
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 5000;
    static constexpr uint32_t LIGHT_UPDATE_INTERVAL_MS = 20;

    void bringUpCore();
    void bringUpComms();
    void waitForConfiguration();
    void bringUpSensors();
    void updateLighting();
    void publishHeartbeat();

    std::vector<std::unique_ptr<SensorBase>> activeSensors_;
    RS485Transceiver rs485Transceiver_;
    LightingEngine lightingEngine_;
    uint32_t lastHeartbeatMs_ = 0;
    uint32_t lastLightUpdateMs_ = 0;
    uint32_t appliedLightConfigVersion_ = 0;
};

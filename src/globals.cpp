#include "../lib/globals.h"

namespace globals {
    std::vector<SensorConfig> sensors;
    std::vector<float> offsets;
    //volatile SystemState state = SystemState::WAIT_CONFIG;

    std::vector<LightConfig> lights;
    std::vector<LightState> lightStates;
    bool lightsConfigured = false;
}
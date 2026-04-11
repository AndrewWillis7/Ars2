#include "SensorFactory.h"

#include <color_sensor.h>
#include <optical_sensor.h>

namespace SensorFactory {

std::unique_ptr<SensorBase> create(
    const RuntimeConfig::SensorConfig& config,
    const std::array<float, 6>& offsets) {
    if (config.type == "COLOR") {
        return std::unique_ptr<SensorBase>(
            new ColorSensor(config.name.c_str(), config.port));
    }

    if (config.type == "OPTICAL") {
        if (config.name == "OPTL") {
            return std::unique_ptr<SensorBase>(new OpticalSensor(
                config.name.c_str(),
                config.port,
                offsets[0],
                offsets[1],
                offsets[2]));
        }

        if (config.name == "OPTR") {
            return std::unique_ptr<SensorBase>(new OpticalSensor(
                config.name.c_str(),
                config.port,
                offsets[3],
                offsets[4],
                offsets[5]));
        }
    }

    return nullptr;
}

}  // namespace SensorFactory

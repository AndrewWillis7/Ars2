#pragma once

#include <RuntimeConfig.h>
#include <sensor_base.h>

#include <array>
#include <memory>

namespace SensorFactory {

std::unique_ptr<SensorBase> create(
    const RuntimeConfig::SensorConfig& config,
    const std::array<float, 6>& offsets);

}  // namespace SensorFactory

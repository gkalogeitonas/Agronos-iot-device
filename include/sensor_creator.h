#pragma once

#include "sensor.h"

// Templated creator helper to be used by each sensor implementation when registering
// with the factory. Requires sensor classes exposing a constructor (int pin, const char* uuid)
// For sensors that don't match that signature, define a small wrapper in the cpp file.

template <typename T>
SensorBase* create_sensor_impl(const SensorConfig& cfg) {
    return new T(cfg.pin, cfg.uuid);
}

// Specialization for sensors that don't accept pin+uuid can be provided in their own files.

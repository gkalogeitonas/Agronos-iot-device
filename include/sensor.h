#pragma once

#include <stddef.h>

// Forward declare SensorConfig (defined in config.h)
struct SensorConfig;

class SensorBase {
public:
    virtual ~SensorBase() = default;
    virtual const char* uuid() const = 0;
    // read a single float value from the sensor, return true on success
    virtual bool read(float &out) = 0;
};

// Creator function type used by the registry
using CreatorFunc = SensorBase*(*)(const SensorConfig& cfg);

// Registration API: sensor implementation files should call this (via a static
// registrar) to register themselves. This lets the factory create instances
// without modifying the factory code when new sensor classes are added.
bool registerSensorFactory(const char* name, CreatorFunc creator);

// Create a sensor instance by name using the registered creators. Returns
// nullptr if no creator is found.
SensorBase* createSensorByType(const char* name, const SensorConfig& cfg);

// Factory helpers (implemented in src/sensor_factory.cpp)
SensorBase** createSensors(size_t &outCount);
void destroySensors(SensorBase** arr, size_t count);

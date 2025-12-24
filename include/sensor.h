#pragma once

#include <stddef.h>
#include <vector>
#include <memory>

// Sensor descriptor used by the sensor factory
struct SensorConfig {
    const char* type;        // e.g. "DHT11_TEMP", "DHT11_HUM", "DS18B20", "SIMULATED"
    int pin;                 // pin or -1 if not applicable
    const char* uuid;        // sensor uuid used by the server
    const char* displayName; // human-readable name for UI display (e.g. "Temperature", "Humidity")
};

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
std::vector<std::unique_ptr<SensorBase>> createSensors();

#pragma once

#include <stddef.h>

class SensorBase {
public:
    virtual ~SensorBase() = default;
    virtual const char* uuid() const = 0;
    // read a single float value from the sensor, return true on success
    virtual bool read(float &out) = 0;
};

// Factory helpers
SensorBase** createSensors(size_t &outCount);
void destroySensors(SensorBase** arr, size_t count);

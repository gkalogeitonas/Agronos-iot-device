#include "sensor.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>

// Simple registry implementation moved to its own file
struct RegistryEntry {
    const char* name;
    CreatorFunc creator;
    RegistryEntry* next;
};

static RegistryEntry* registryHead = nullptr;

bool registerSensorFactory(const char* name, CreatorFunc creator) {
    RegistryEntry* e = (RegistryEntry*)malloc(sizeof(RegistryEntry));
    if (!e) return false;
    e->name = name;
    e->creator = creator;
    e->next = registryHead;
    registryHead = e;
    return true;
}

SensorBase* createSensorByType(const char* name, const SensorConfig& cfg) {
    for (RegistryEntry* e = registryHead; e != nullptr; e = e->next) {
        if (strcmp(e->name, name) == 0) return e->creator(cfg);
    }
    return nullptr;
}

SensorBase** createSensors(size_t &outCount) {
    outCount = SENSOR_CONFIG_COUNT;
    SensorBase** arr = (SensorBase**)malloc(sizeof(SensorBase*) * outCount);
    if (!arr) return nullptr;

    for (size_t i = 0; i < outCount; ++i) {
        const SensorConfig &cfg = SENSOR_CONFIGS[i];
        SensorBase* inst = createSensorByType(cfg.type, cfg);
        if (!inst) {
            // Unknown type -> fallback to registered SIMULATED creator if present
            inst = createSensorByType("SIMULATED", cfg);
            if (!inst) {
                // As a last resort leave a null pointer; caller must handle this case.
                inst = nullptr;
            }
        }
        arr[i] = inst;
    }
    return arr;
}

void destroySensors(SensorBase** arr, size_t count) {
    if (!arr) return;
    for (size_t i = 0; i < count; ++i) {
        delete arr[i];
    }
    free(arr);
}

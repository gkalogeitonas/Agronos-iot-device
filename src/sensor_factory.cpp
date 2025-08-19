#include "sensor.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>
#include <vector>
#include <memory>
#include <algorithm>

// Registry using a vector of pairs to avoid manual malloc and linked list
static std::vector<std::pair<const char*, CreatorFunc>>& registry() {
    static std::vector<std::pair<const char*, CreatorFunc>> r;
    return r;
}

bool registerSensorFactory(const char* name, CreatorFunc creator) {
    registry().emplace_back(name, creator);
    return true;
}

SensorBase* createSensorByType(const char* name, const SensorConfig& cfg) {
    for (auto &p : registry()) {
        if (strcmp(p.first, name) == 0) return p.second(cfg);
    }
    return nullptr;
}

std::vector<std::unique_ptr<SensorBase>> createSensors() {
    std::vector<std::unique_ptr<SensorBase>> vec;
    vec.reserve(SENSOR_CONFIG_COUNT);

    for (size_t i = 0; i < SENSOR_CONFIG_COUNT; ++i) {
        const SensorConfig &cfg = SENSOR_CONFIGS[i];
        SensorBase* inst = createSensorByType(cfg.type, cfg);
        if (!inst) {
            // Unknown type -> fallback to registered SIMULATED creator if present
            inst = createSensorByType("SIMULATED", cfg);
            if (!inst) {
                // skip sensor if we cannot create it
                continue;
            }
        }
        vec.emplace_back(inst);
    }
    return vec;
}

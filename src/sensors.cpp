#include "sensor.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>

// Include DHT only when building DHT sensors
#include <DHT.h>

// Simple registry implementation
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

// DHT11 temperature reader
class DHT11TemperatureReader : public SensorBase {
public:
    DHT11TemperatureReader(int pin, const char* uid)
    : pin_(pin), uuid_(uid), dht_(pin, DHT11) {
        dht_.begin();
    }
    const char* uuid() const override { return uuid_; }
    bool read(float &out) override {
        float t = dht_.readTemperature();
        if (isnan(t)) return false;
        out = t;
        return true;
    }
private:
    int pin_;
    const char* uuid_;
    DHT dht_;
};

// DHT11 humidity reader
class DHT11HumidityReader : public SensorBase {
public:
    DHT11HumidityReader(int pin, const char* uid)
    : pin_(pin), uuid_(uid), dht_(pin, DHT11) {
        dht_.begin();
    }
    const char* uuid() const override { return uuid_; }
    bool read(float &out) override {
        float h = dht_.readHumidity();
        if (isnan(h)) return false;
        out = h;
        return true;
    }
private:
    int pin_;
    const char* uuid_;
    DHT dht_;
};

// Simulated sensor for testing/fallback
class SimulatedSensor : public SensorBase {
public:
    SimulatedSensor(const char* uid) : uuid_(uid) {}
    const char* uuid() const override { return uuid_; }
    bool read(float &out) override {
        static float v = 20.0f;
        v += 0.13f;
        out = v;
        return true;
    }
private:
    const char* uuid_;
};

// Templated creator to avoid one-off wrapper functions for every sensor class
template <typename T>
SensorBase* create_sensor(const SensorConfig& cfg) {
    return new T(cfg.pin, cfg.uuid);
}

// Specialize for sensors that don't follow the (int pin, const char* uuid) constructor
template <>
SensorBase* create_sensor<SimulatedSensor>(const SensorConfig& cfg) {
    return new SimulatedSensor(cfg.uuid);
}

// Static registrars to register built-in sensor types using the templated creator
static bool _reg1 = registerSensorFactory("DHT11TemperatureReader", create_sensor<DHT11TemperatureReader>);
static bool _reg2 = registerSensorFactory("DHT11HumidityReader", create_sensor<DHT11HumidityReader>);
static bool _reg3 = registerSensorFactory("SIMULATED", create_sensor<SimulatedSensor>);

SensorBase** createSensors(size_t &outCount) {
    outCount = SENSOR_CONFIG_COUNT;
    SensorBase** arr = (SensorBase**)malloc(sizeof(SensorBase*) * outCount);
    if (!arr) return nullptr;

    for (size_t i = 0; i < outCount; ++i) {
        const SensorConfig &cfg = SENSOR_CONFIGS[i];
        SensorBase* inst = createSensorByType(cfg.type, cfg);
        if (!inst) {
            // Unknown type -> fallback to simulated
            inst = create_sensor<SimulatedSensor>(cfg);
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

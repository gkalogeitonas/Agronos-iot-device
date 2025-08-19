#include "sensor.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>

// Include DHT only when building DHT sensors
#include <DHT.h>

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

SensorBase** createSensors(size_t &outCount) {
    outCount = SENSOR_CONFIG_COUNT;
    SensorBase** arr = (SensorBase**)malloc(sizeof(SensorBase*) * outCount);
    for (size_t i = 0; i < outCount; ++i) {
        const SensorConfig &cfg = SENSOR_CONFIGS[i];
        if (strcmp(cfg.type, "DHT11_TEMP") == 0) {
            arr[i] = new DHT11TemperatureReader(cfg.pin, cfg.uuid);
        } else if (strcmp(cfg.type, "DHT11_HUM") == 0) {
            arr[i] = new DHT11HumidityReader(cfg.pin, cfg.uuid);
        } else if (strcmp(cfg.type, "SIMULATED") == 0) {
            arr[i] = new SimulatedSensor(cfg.uuid);
        } else {
            // Unknown type -> simulated fallback
            arr[i] = new SimulatedSensor(cfg.uuid);
        }
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

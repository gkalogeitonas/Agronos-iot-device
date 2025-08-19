#include "sensor.h"
#include "config.h"
#include "dht_shared.h"
#include "sensor_creator.h"
#include <Arduino.h>

class DHT11HumidityReader : public SensorBase {
public:
    DHT11HumidityReader(int pin, const char* uid)
    : pin_(pin), uuid_(uid), dht_(getSharedDht(pin)) {}
    const char* uuid() const override { return uuid_; }
    bool read(float &out) override {
        if (!dht_) return false;
        float h = dht_->readHumidity();
        if (isnan(h)) return false;
        out = h;
        return true;
    }
private:
    int pin_;
    const char* uuid_;
    DHT* dht_;
};

static bool _reg = registerSensorFactory("DHT11HumidityReader", create_sensor_impl<DHT11HumidityReader>);

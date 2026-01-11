#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"
#include <Arduino.h>
#include <DHT20.h>
#include <Wire.h>

/**
 * DHT20 Temperature and Humidity Sensor (I2C)
 * - Uses robtillaart/DHT20 library
 * - Default I2C address is 0x38
 * - Multiple readers share the same DHT20 instance to avoid I2C conflicts
 */

static DHT20* getSharedDHT20() {
    static DHT20* dht = nullptr;
    if (!dht) {
        // Initialize I2C with pins defined in config.h
        Wire.begin(I2C_SDA, I2C_SCL); 
        dht = new DHT20(&Wire);
        if (!dht->begin()) {
            Serial.println("DHT20: Failed to initialize sensor!");
            return nullptr;
        }
        Serial.println("DHT20: Sensor initialized");
    }
    return dht;
}

class DHT20TemperatureReader : public SensorBase {
public:
    DHT20TemperatureReader(int pin, const char* uid)
    : uuid_(uid), dht_(getSharedDHT20()) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        if (!dht_) return false;
        
        // The DHT20 library read() returns DHT20_OK (0) on success
        int status = dht_->read();
        if (status != DHT20_OK) {
            Serial.print("DHT20 Temp Read Error: ");
            Serial.println(status);
            return false;
        }
        
        out = dht_->getTemperature();
        return true;
    }
    
private:
    const char* uuid_;
    DHT20* dht_;
};

class DHT20HumidityReader : public SensorBase {
public:
    DHT20HumidityReader(int pin, const char* uid)
    : uuid_(uid), dht_(getSharedDHT20()) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        if (!dht_) return false;
        
        int status = dht_->read();
        if (status != DHT20_OK) {
            Serial.print("DHT20 Hum Read Error: ");
            Serial.println(status);
            return false;
        }
        
        out = dht_->getHumidity();
        return true;
    }
    
private:
    const char* uuid_;
    DHT20* dht_;
};

// Register the factory creators
static bool _reg_temp = registerSensorFactory("DHT20TemperatureReader", create_sensor_impl<DHT20TemperatureReader>);
static bool _reg_hum = registerSensorFactory("DHT20HumidityReader", create_sensor_impl<DHT20HumidityReader>);

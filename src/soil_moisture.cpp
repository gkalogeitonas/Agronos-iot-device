#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"
#include <Arduino.h>

/**
 * DFRobot Capacitive Soil Moisture Sensor (SEN0193)
 * - Analog output: 0-3V
 * - Connected to GPIO32 (ADC1_CH4)
 * - Requires calibration (air value = dry, water value = wet)
 * - Returns moisture percentage: 0% (dry) to 100% (wet)
 */
class SoilMoistureSensor : public SensorBase {
public:
    SoilMoistureSensor(int pin, const char* uid)
    : pin_(pin), uuid_(uid) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        // Average multiple readings for stability
        const int numSamples = 10;
        long sum = 0;
        
        for (int i = 0; i < numSamples; i++) {
            sum += analogRead(pin_);
            delay(10);
        }
        
        int rawValue = sum / numSamples;
        
        // Calibration values (MUST be determined empirically)
        // These are estimates scaled from Arduino 10-bit to ESP32 12-bit ADC
        // To calibrate:
        //   1. Expose sensor to air, note the raw value -> update airValue
        //   2. Submerge in water (to limit line), note raw value -> update waterValue
        const int airValue = SOIL_MOISTURE_AIR_VALUE;    // Dry soil (high value, low capacitance)
        const int waterValue = SOIL_MOISTURE_WATER_VALUE;  // Wet soil (low value, high capacitance)
        
        // Clamp to calibrated range
        if (rawValue > airValue) rawValue = airValue;
        if (rawValue < waterValue) rawValue = waterValue;
        
        // Convert to percentage: 100% = wet (waterValue), 0% = dry (airValue)
        float moisturePercent = 100.0f * (float)(airValue - rawValue) / (float)(airValue - waterValue);
        
        out = moisturePercent;
        return true;
    }
    
private:
    int pin_;
    const char* uuid_;
};

// Auto-register with factory using standard creator template
static bool _reg = registerSensorFactory("SoilMoistureSensor", create_sensor_impl<SoilMoistureSensor>);

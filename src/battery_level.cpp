#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"
#include <Arduino.h>

/**
 * DFRobot FireBeetle 2 ESP32-C6 Battery Level Sensor
 * - Reads battery voltage via ADC0 (GPIO pin 0)
 * - Built-in voltage divider (2:1 ratio) on the board
 * - 12-bit ADC resolution (0-4096)
 * - Returns battery percentage based on LiPo discharge curve
 * 
 * Battery Specifications:
 * - Type: Polymer Lithium Ion Battery (single-cell LiPo)
 * - Nominal Voltage: 3.7V
 * - Capacity: 1200mAh
 * 
 * Voltage ranges:
 * - 4.2V = 100% (fully charged)
 * - 3.7V = ~50% (nominal voltage)
 * - 3.0V = 0% (cutoff voltage)
 */
class BatteryLevelSensor : public SensorBase {
public:
    BatteryLevelSensor(int pin, const char* uid)
    : adc_pin_(0), uuid_(uid) {
        // Battery voltage is always on ADC0 for FireBeetle 2 ESP32-C6
        // Pin parameter is ignored but kept for API compatibility
        analogReadResolution(12); // Set 12-bit resolution (0-4096)
    }
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        // Average multiple readings for stability
        const int numSamples = 10;
        long sum = 0;
        
        for (int i = 0; i < numSamples; i++) {
            sum += analogRead(adc_pin_);
            delay(10);
        }
        
        int rawValue = sum / numSamples;
        
        // Convert ADC value to millivolts (ESP32-C6 ADC reference is typically 3.3V)
        // ADC range: 0-4095 maps to 0-3300mV
        float adcMillivolts = (rawValue * 3300.0f) / 4095.0f;
        
        // Apply voltage divider correction (board has 2:1 divider)
        // Actual battery voltage is 2x the ADC reading
        float batteryMillivolts = adcMillivolts * 2.0f;
        
        // Convert to percentage based on LiPo discharge curve
        // 4200mV = 100%, 3000mV = 0%
        float batteryPercent = voltageToBatteryPercent(batteryMillivolts);
        
        out = batteryPercent;
        return true;
    }
    
private:
    int adc_pin_;
    const char* uuid_;
    
    /**
     * Convert battery voltage to percentage using LiPo discharge curve
     * Based on typical single-cell LiPo (3.7V nominal) discharge characteristics
     */
    float voltageToBatteryPercent(float millivolts) const {
        const float voltage = millivolts / 1000.0f; // Convert to volts
        
        // LiPo voltage-to-percentage mapping (approximate)
        // These values provide a reasonable linear approximation
        const float VOLTAGE_MAX = 4.2f;  // 100% - fully charged
        const float VOLTAGE_MIN = 3.0f;  // 0% - cutoff voltage
        
        // Clamp to valid range
        if (voltage >= VOLTAGE_MAX) return 100.0f;
        if (voltage <= VOLTAGE_MIN) return 0.0f;
        
        // For more accuracy, use a piecewise linear approximation of the LiPo discharge curve:
        // 4.2V-4.0V: 100%-90% (rapid drop at top)
        // 4.0V-3.7V: 90%-50% (gradual decline)
        // 3.7V-3.4V: 50%-20% (moderate decline)
        // 3.4V-3.0V: 20%-0% (rapid drop at bottom)
        
        if (voltage >= 4.0f) {
            // 100% to 90%: 4.2V to 4.0V
            return 90.0f + ((voltage - 4.0f) / 0.2f) * 10.0f;
        } else if (voltage >= 3.7f) {
            // 90% to 50%: 4.0V to 3.7V
            return 50.0f + ((voltage - 3.7f) / 0.3f) * 40.0f;
        } else if (voltage >= 3.4f) {
            // 50% to 20%: 3.7V to 3.4V
            return 20.0f + ((voltage - 3.4f) / 0.3f) * 30.0f;
        } else {
            // 20% to 0%: 3.4V to 3.0V
            return ((voltage - 3.0f) / 0.4f) * 20.0f;
        }
    }
};

// Auto-register with factory using standard creator template
static bool _reg = registerSensorFactory("BatteryLevelSensor", create_sensor_impl<BatteryLevelSensor>);

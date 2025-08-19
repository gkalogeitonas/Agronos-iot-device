#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"

class SimulatedSensor : public SensorBase {
public:
    SimulatedSensor(int pin_unused, const char* uid) : uuid_(uid) {}
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

static bool _reg = registerSensorFactory("SIMULATED", create_sensor_impl<SimulatedSensor>);

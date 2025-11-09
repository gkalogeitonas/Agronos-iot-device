#pragma once

#include <Arduino.h>
#include "storage.h"

// Forward declaration
class MqttClient;

struct SensorReading {
    const char* uuid;
    float value;
};

class DataSender {
public:
    DataSender(Storage &storage, const char* baseUrl);

    // Set MQTT client for MQTT support (optional)
    void setMqttClient(MqttClient* client);

    // Send an array of SensorReading { uuid, value }
    bool sendReadings(const SensorReading* readings, size_t count);

    // Send values paired with explicit UUIDs
    bool sendValuesWithUuids(const char* uuids[], const float* values, size_t count);

    // Backwards-compatible helper that uses SENSOR_UUIDS from config.h
    bool sendValues(const float* values, size_t count);

private:
    Storage &storage;
    const char* baseUrl;
    MqttClient* mqttClient; // Optional MQTT client

    String buildUrl() const;
    bool postJson(const String &json, const String &token);
};

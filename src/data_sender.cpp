#include "data_sender.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "mqtt_client.h"

DataSender::DataSender(Storage &storage, const char* baseUrl)
: storage(storage), baseUrl(baseUrl), mqttClient(nullptr) {}

void DataSender::setMqttClient(MqttClient* client) {
    mqttClient = client;
    Serial.println("MQTT client linked to DataSender");
}

String DataSender::buildUrl() const {
    return String(baseUrl) + "/api/v1/device/data";
}

bool DataSender::postJson(const String &json, const String &token) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot send data");
        return false;
    }

    HTTPClient http;
    String url = buildUrl();
    Serial.print("Posting data to: "); Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + token);

    int code = http.POST(json);
    Serial.print("HTTP code: "); Serial.println(code);
    String resp = http.getString();
    Serial.print("Response: "); Serial.println(resp);

    http.end();
    return (code >= 200 && code < 300);
}

bool DataSender::sendReadings(const SensorReading* readings, size_t count) {
    if (count == 0 || readings == nullptr) return false;

    // Try MQTT first if enabled and credentials are available (runtime check)
    if (MQTT_ENABLED && mqttClient != nullptr && storage.hasMqttCredentials()) {
        Serial.println("Attempting to send data via MQTT...");

        // Try to connect if not already connected
        if (mqttClient->isConnected() || mqttClient->connect()) {
            bool success = mqttClient->publishSensorData(readings, count);
            if (success) {
                Serial.println("Data sent successfully via MQTT");
                return true;
            } else {
                Serial.println("MQTT publish failed, falling back to HTTP");
            }
        } else {
            Serial.println("MQTT connection failed, falling back to HTTP");
        }
    }

    // Fallback to HTTP (or use HTTP if MQTT is not enabled/configured)
    Serial.println("Sending data via HTTP...");
    
    String token = storage.getToken();
    if (token.length() == 0) {
        Serial.println("No auth token available");
        return false;
    }

    // Estimate JSON capacity conservatively
    const size_t capacity = JSON_ARRAY_SIZE(count) + count * JSON_OBJECT_SIZE(2) + 256;
    DynamicJsonDocument doc(capacity);

    JsonArray sensors = doc.createNestedArray("sensors");
    // Round values to a fixed number of decimal places before serializing
    constexpr int VALUE_PRECISION = 2; // number of decimal places (e.g., 2 -> 22.50)
    const float factor = powf(10.0f, VALUE_PRECISION);
    for (size_t i = 0; i < count; ++i) {
        JsonObject obj = sensors.createNestedObject();
        obj["uuid"] = readings[i].uuid;
        float rounded = roundf(readings[i].value * factor) / factor;
        obj["value"] = rounded;
    }

    String out;
    serializeJson(doc, out);
    bool result = postJson(out, token);
    
    if (result) {
        Serial.println("Data sent successfully via HTTP");
    } else {
        Serial.println("HTTP send failed");
    }
    
    return result;
}

bool DataSender::sendValuesWithUuids(const char* uuids[], const float* values, size_t count) {
    if (!uuids || !values || count == 0) return false;
    // build temporary readings on stack
    SensorReading tmp[count];
    for (size_t i = 0; i < count; ++i) {
        tmp[i].uuid = uuids[i];
        tmp[i].value = values[i];
    }
    return sendReadings(tmp, count);
}

bool DataSender::sendValues(const float* values, size_t count) {
    if (!values || count == 0) return false;
    if (count > SENSOR_CONFIG_COUNT) {
        Serial.println("sendValues: count exceeds SENSOR_CONFIG_COUNT");
        return false;
    }

    // Build UUID array from SENSOR_CONFIGS (backwards-compatible behavior)
    const char** uuids = (const char**)malloc(sizeof(const char*) * count);
    if (!uuids) return false;
    for (size_t i = 0; i < count; ++i) {
        uuids[i] = SENSOR_CONFIGS[i].uuid;
    }
    bool result = sendValuesWithUuids(uuids, values, count);
    free(uuids);
    return result;
}

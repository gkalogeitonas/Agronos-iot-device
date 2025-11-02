#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "storage.h"
#include "data_sender.h" // For SensorReading struct

class MqttClient {
public:
    MqttClient(Storage &storage, const char* deviceUuid);
    
    // Connection management
    bool connect();
    bool isConnected();
    void disconnect();
    void loop(); // Must be called regularly
    
    // Publish sensor data
    bool publishSensorData(const SensorReading* readings, size_t count);
    
    // Publish device status
    bool publishStatus(const char* status);
    
private:
    Storage &storage;
    WiFiClient wifiClient;
    WiFiClientSecure secureClient;
    PubSubClient mqttClient;
    
    const char* deviceUuid;
    MqttCredentials credentials;
    bool credentialsLoaded;
    
    unsigned long lastReconnectAttempt;
    
    String buildTopic(const char* topicTemplate) const;
    bool loadCredentials();
    bool reconnect();
    
    // Callback for incoming messages
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

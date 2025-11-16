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
    
    // Publish sensor data payload (JSON string built by DataSender)
    bool publishSensorDataPayload(const char* payload);
    
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
    
    String buildTopic(const char* topicTemplate) const;
    bool loadCredentials();
    
    // Callback for incoming messages
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

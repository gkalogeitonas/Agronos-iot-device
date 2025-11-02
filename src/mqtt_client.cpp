#include "mqtt_client.h"
#include <ArduinoJson.h>
#include "config.h"

MqttClient::MqttClient(Storage &storage, const char* deviceUuid)
: storage(storage), 
  deviceUuid(deviceUuid),
  credentialsLoaded(false),
  lastReconnectAttempt(0) {
    
    // Initialize MQTT client with appropriate WiFi client
    #ifdef AGRONOS_MQTT_USE_TLS
    if (AGRONOS_MQTT_USE_TLS) {
        secureClient.setInsecure(); // For testing; use proper certificates in production
        mqttClient.setClient(secureClient);
    } else {
    #endif
        mqttClient.setClient(wifiClient);
    #ifdef AGRONOS_MQTT_USE_TLS
    }
    #endif
    
    mqttClient.setCallback(MqttClient::mqttCallback);
    mqttClient.setKeepAlive(AGRONOS_MQTT_KEEPALIVE);
}

bool MqttClient::loadCredentials() {
    if (credentialsLoaded && credentials.isValid) {
        return true;
    }
    
    credentialsLoaded = storage.getMqttCredentials(credentials);
    return credentialsLoaded && credentials.isValid;
}

String MqttClient::buildTopic(const char* topicTemplate) const {
    char topic[128];
    snprintf(topic, sizeof(topic), topicTemplate, deviceUuid);
    return String(topic);
}

bool MqttClient::connect() {
    if (!loadCredentials()) {
        Serial.println("Cannot connect to MQTT: No credentials available");
        return false;
    }
    
    if (mqttClient.connected()) {
        return true;
    }
    
    // Set MQTT server
    mqttClient.setServer(credentials.server.c_str(), AGRONOS_MQTT_PORT);
    
    // Generate client ID
    String clientId = String("agronos-") + deviceUuid;
    
    Serial.print("Connecting to MQTT broker: ");
    Serial.print(credentials.server);
    Serial.print(":");
    Serial.println(AGRONOS_MQTT_PORT);
    
    // Attempt connection
    bool connected = mqttClient.connect(
        clientId.c_str(),
        credentials.username.c_str(),
        credentials.password.c_str(),
        nullptr,  // willTopic
        0,        // willQos
        false,    // willRetain
        nullptr,  // willMessage
        AGRONOS_MQTT_CLEAN_SESSION
    );
    
    if (connected) {
        Serial.println("MQTT connected successfully");
        
        // Subscribe to command topic
        String cmdTopic = buildTopic(AGRONOS_MQTT_TOPIC_COMMAND);
        if (mqttClient.subscribe(cmdTopic.c_str(), 1)) {
            Serial.print("Subscribed to: "); Serial.println(cmdTopic);
        }
        
        return true;
    } else {
        Serial.print("MQTT connection failed, state: ");
        Serial.println(mqttClient.state());
        return false;
    }
}

bool MqttClient::reconnect() {
    unsigned long now = millis();
    if (now - lastReconnectAttempt < AGRONOS_MQTT_RECONNECT_DELAY) {
        return false;
    }
    
    lastReconnectAttempt = now;
    Serial.println("Attempting MQTT reconnection...");
    return connect();
}

bool MqttClient::isConnected() {
    return mqttClient.connected();
}

void MqttClient::disconnect() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
        Serial.println("MQTT disconnected");
    }
}

void MqttClient::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool MqttClient::publishSensorData(const SensorReading* readings, size_t count) {
    if (!isConnected()) {
        Serial.println("MQTT not connected, cannot publish sensor data");
        return false;
    }
    
    if (count == 0 || readings == nullptr) {
        Serial.println("No sensor readings to publish");
        return false;
    }
    
    // Build JSON payload
    const size_t capacity = JSON_ARRAY_SIZE(count) + count * JSON_OBJECT_SIZE(2) + 256;
    DynamicJsonDocument doc(capacity);
    
    doc["device_id"] = deviceUuid;
    doc["timestamp"] = millis() / 1000; // seconds since boot
    
    JsonArray sensors = doc.createNestedArray("sensors");
    for (size_t i = 0; i < count; ++i) {
        JsonObject obj = sensors.createNestedObject();
        obj["uuid"] = readings[i].uuid;
        obj["value"] = readings[i].value;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = buildTopic(AGRONOS_MQTT_TOPIC_DATA);
    
    Serial.print("Publishing to MQTT topic: "); Serial.println(topic);
    Serial.print("Payload: "); Serial.println(payload);
    
    bool published = mqttClient.publish(topic.c_str(), payload.c_str(), false);
    
    if (published) {
        Serial.println("MQTT publish successful");
    } else {
        Serial.println("MQTT publish failed");
    }
    
    return published;
}

bool MqttClient::publishStatus(const char* status) {
    if (!isConnected()) {
        return false;
    }
    
    String topic = buildTopic(AGRONOS_MQTT_TOPIC_STATUS);
    return mqttClient.publish(topic.c_str(), status, false);
}

void MqttClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("MQTT message received on topic: ");
    Serial.println(topic);
    
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.print("Payload: ");
    Serial.println(message);
    
    // TODO: Handle commands here in future
}

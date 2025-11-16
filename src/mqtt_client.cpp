#include "mqtt_client.h"
#include <ArduinoJson.h>
#include "config.h"

MqttClient::MqttClient(Storage &storage, const char* deviceUuid)
: storage(storage), 
  deviceUuid(deviceUuid),
    credentialsLoaded(false) {
    
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
    
    // No callback/subscriptions needed for this device (publish-only)
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
        
    // No subscription needed for publish-only device
        
        return true;
    } else {
        Serial.print("MQTT connection failed, state: ");
        Serial.println(mqttClient.state());
        return false;
    }
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

void MqttClient::process() {
    // Run a single pass of the underlying client to let the TCP/MQTT stack settle
    if (mqttClient.connected()) {
        mqttClient.loop();
        delay(50);
    }
}

bool MqttClient::publishSensorDataPayload(const char* payload) {
    if (!isConnected()) {
        Serial.println("MQTT not connected, cannot publish sensor data");
        return false;
    }

    if (!payload) {
        Serial.println("Empty payload passed to publishSensorDataPayload");
        return false;
    }

    String topic = buildTopic(AGRONOS_MQTT_TOPIC_DATA);
    Serial.print("Publishing to MQTT topic: "); Serial.println(topic);
    Serial.print("Payload: "); Serial.println(payload);

    bool published = mqttClient.publish(topic.c_str(), payload, false);

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

// mqttCallback removed: device is publish-only and does not subscribe to topics

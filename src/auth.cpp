#include "auth.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

AuthManager::AuthManager(Storage &storage, const char* baseUrl, const char* uuid, const char* secret, unsigned long retryIntervalMs)
: storage(storage), baseUrl(baseUrl), uuid(uuid), secret(secret), retryIntervalMs(retryIntervalMs), lastAttempt(0) {}

void AuthManager::begin() {
  // nothing for now
}

String AuthManager::getSavedToken() {
  return storage.getToken();
}

bool AuthManager::performAuthRequest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi, skipping authentication");
    return false;
  }

  HTTPClient http;
  String url = String(baseUrl) + "/api/v1/device/login";
  Serial.print("Authenticating to: "); Serial.println(url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String payload = String("{\"uuid\":\"") + uuid + "\",\"secret\":\"" + secret + "\"}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.print("HTTP response code: "); Serial.println(httpCode);
    String resp = http.getString();
    Serial.print("Response: "); Serial.println(resp);

    // Use ArduinoJson to parse response safely
    // Allocate a document with a reasonable size for expected responses
    const size_t capacity = 1024;
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
      Serial.print("Failed to parse JSON response: "); Serial.println(err.c_str());
      http.end();
      return false;
    }

    // Successful HTTP codes (2xx) — try to extract token
    if (httpCode >= 200 && httpCode < 300) {
      if (doc.containsKey("token")) {
        const char* token = doc["token"];
        if (token != nullptr) {
          storage.setToken(String(token));
          http.end();
          return true;
        }
      }
      // If no token field, try to print a message if present
      if (doc.containsKey("message")) {
        const char* message = doc["message"];
        if (message) {
          Serial.print("Server message: "); Serial.println(message);
        }
      }

      http.end();
      return false;
    }

    // Non-2xx responses — print message if available
    if (doc.containsKey("message")) {
      const char* message = doc["message"];
      if (message) {
        Serial.print("Server message: "); Serial.println(message);
      }
    }

    http.end();
    return false;
  } else {
    Serial.print("Request failed, error: "); Serial.println(http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

bool AuthManager::tryAuthenticateOnce() {
  lastAttempt = millis();
  return performAuthRequest();
}

void AuthManager::loop() {
  if (WiFi.status() != WL_CONNECTED) return;
  String token = storage.getToken();
  if (token.length() > 0) return; // already have token
  unsigned long now = millis();
  if (now - lastAttempt >= retryIntervalMs) {
    lastAttempt = now;
    performAuthRequest();
  }
}

bool AuthManager::hasMqttCredentials() {
  return storage.hasMqttCredentials();
}

bool AuthManager::performMqttCredentialsRequest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot fetch MQTT credentials");
    return false;
  }
  
  String token = storage.getToken();
  if (token.length() == 0) {
    Serial.println("No JWT token available for MQTT provisioning");
    return false;
  }
  
  HTTPClient http;
  String url = String(baseUrl) + "/api/v1/device/mqtt-credentials";
  Serial.print("Fetching MQTT credentials from: "); Serial.println(url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + token);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.print("HTTP response code: "); Serial.println(httpCode);
    String response = http.getString();
    Serial.print("Response: "); Serial.println(response);
    
    if (httpCode >= 200 && httpCode < 300) {
      // Parse JSON response
      const size_t capacity = JSON_OBJECT_SIZE(10) + 512;
      DynamicJsonDocument doc(capacity);
      DeserializationError err = deserializeJson(doc, response);
      
      if (err) {
        Serial.print("Failed to parse MQTT credentials JSON: ");
        Serial.println(err.c_str());
        http.end();
        return false;
      }
      
      // Extract credentials
      if (!doc.containsKey("mqtt_broker_url") || 
          !doc.containsKey("username") || 
          !doc.containsKey("password")) {
        Serial.println("Missing MQTT credential fields in response");
        http.end();
        return false;
      }
      
      String server = doc["mqtt_broker_url"].as<String>();
      String username = doc["username"].as<String>();
      String password = doc["password"].as<String>();
      
      if (server.length() == 0 || username.length() == 0 || password.length() == 0) {
        Serial.println("Empty MQTT credential fields");
        http.end();
        return false;
      }
      
      // Store credentials
      storage.setMqttCredentials(server, username, password);
      
      Serial.println("MQTT credentials obtained successfully:");
      Serial.print("  Server: "); Serial.println(server);
      Serial.print("  Username: "); Serial.println(username);
      Serial.println("  Password: "); Serial.println(password); // Print actual password

      http.end();
      return true;
    } else {
      Serial.println("Failed to fetch MQTT credentials - non-2xx response");
      http.end();
      return false;
    }
  } else {
    Serial.print("HTTP request failed: ");
    Serial.println(http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

bool AuthManager::fetchMqttCredentials() {
  return performMqttCredentialsRequest();
}

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

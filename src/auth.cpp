#include "auth.h"
#include <HTTPClient.h>
#include <WiFi.h>

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

    // Try to extract token from response (simple parsing without ArduinoJson)
    int tokenKey = resp.indexOf("\"token\"");
    if (tokenKey >= 0) {
      int colon = resp.indexOf(':', tokenKey);
      if (colon >= 0) {
        int start = resp.indexOf('"', colon);
        int end = resp.indexOf('"', start + 1);
        if (start >= 0 && end > start) {
          String token = resp.substring(start + 1, end);
          storage.setToken(token);
          http.end();
          return true;
        }
      }
    }

    // Check for message field (e.g., invalid credentials)
    int msgKey = resp.indexOf("\"message\"");
    if (msgKey >= 0) {
      int colon = resp.indexOf(':', msgKey);
      int start = resp.indexOf('"', colon);
      int end = resp.indexOf('"', start + 1);
      if (start >= 0 && end > start) {
        String message = resp.substring(start + 1, end);
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

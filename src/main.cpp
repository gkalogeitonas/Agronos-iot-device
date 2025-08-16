/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <WiFi.h>
#include "wifi_portal.h"
#include "storage.h"
#include <HTTPClient.h>

const char* apSsid = "ESP_Config";
const char* apPass = ""; // optional

// Replace global Preferences with Storage instance
Storage storage;
WifiPortal portal(storage, apSsid, apPass);

const char* baseUrl = "https://agronos.kalogeitonas.xyz"; // e.g. "http://example.com" (no trailing slash)
const char* deviceUuid = "Test-Device-1";
const char* deviceSecret = "Test-Device-1";

void tryAutoConnect() {
  String ssid, pass;
  if (!storage.getWifiCreds(ssid, pass)) return;
  if (ssid.length() == 0) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (millis() - start < 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to saved WiFi");
      return;
    }
    delay(200);
  }
  Serial.println("Failed to connect to saved WiFi");
}

void saveAuthToken(const String &token) {
  storage.setToken(token);
  Serial.print("Saved auth token: "); Serial.println(token);
}

String getSavedAuthToken() {
  return storage.getToken();
}

bool authenticateDevice() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi, skipping authentication");
    return false;
  }

  HTTPClient http;
  String url = String(baseUrl) + "/api/v1/device/login";
  Serial.print("Authenticating to: "); Serial.println(url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String payload = String("{\"uuid\":\"") + deviceUuid + "\",\"secret\":\"" + deviceSecret + "\"}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.print("HTTP response code: "); Serial.println(httpCode);
    String resp = http.getString();
    Serial.print("Response: "); Serial.println(resp);

    // Try to extract token from response (simple parsing without ArduinoJson)
    int tokenKey = resp.indexOf("\"token\"");
    if (tokenKey >= 0) {
      int firstQuote = resp.indexOf('"', tokenKey + 7); // find quote after token key
      int secondQuote = resp.indexOf('"', firstQuote + 1);
      int thirdQuote = resp.indexOf('"', secondQuote + 1);
      int fourthQuote = resp.indexOf('"', thirdQuote + 1);
      // Fallback: find first quote after ':'
      int colon = resp.indexOf(':', tokenKey);
      if (colon >= 0) {
        int start = resp.indexOf('"', colon);
        int end = resp.indexOf('"', start + 1);
        if (start >= 0 && end > start) {
          String token = resp.substring(start + 1, end);
          saveAuthToken(token);
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

const unsigned long AUTH_RETRY_INTERVAL_MS = 30000; // try every 30s when no token
unsigned long lastAuthAttempt = 0;

void setup()
{
    Serial.begin(115200);

    // TEST: clear saved WiFi credentials on every boot so portal always starts.
    // Comment out the next line when you no longer want this behavior.
    //storage.setWifiCreds("", "");

    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        portal.start();
    } else {
        Serial.print("IP: "); Serial.println(WiFi.localIP());
        // Attempt authentication once after successful connection
        bool authOk = authenticateDevice();
        if (authOk) {
          Serial.println("Device authenticated successfully");
        } else {
          Serial.println("Device authentication failed");
        }
    }
}

void loop()
{
    portal.handle();

    // If connected and no saved token, attempt authentication periodically
    if (WiFi.status() == WL_CONNECTED) {
      String token = getSavedAuthToken();
      if (token.length() == 0) {
        unsigned long now = millis();
        if (now - lastAuthAttempt >= AUTH_RETRY_INTERVAL_MS) {
          Serial.println("No auth token found, attempting authentication...");
          bool authOk = authenticateDevice();
          lastAuthAttempt = now;
          if (authOk) {
            Serial.println("Authentication succeeded from loop");
          } else {
            Serial.println("Authentication failed from loop");
          }
        }
      } else {
        // Optional: print token occasionally for debug
        Serial.print("Saved token: "); Serial.println(token);
      }
    }

    // Wait a bit before scanning again
    delay(5000);
}
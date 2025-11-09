/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <WiFi.h>
#include "wifi_portal.h"
#include "storage.h"
#include "auth.h"
#include <HTTPClient.h>
#include "config.h"
#include "data_sender.h"
#include <math.h>

#include "sensor.h"
#include <esp_sleep.h>

#include "mqtt_client.h"

const char* apSsid = AP_SSID;
const char* apPass = AP_PASS; // optional

// Replace global Preferences with Storage instance
Storage storage;
WifiPortal portal(storage, apSsid, apPass);

const char* baseUrl = BASE_URL; // e.g. "http://example.com" (no trailing slash)
const char* deviceUuid = DEFAULT_UUID;
const char* deviceSecret = DEFAULT_SECRET;

// Auth manager
AuthManager auth(storage, baseUrl, deviceUuid, deviceSecret, AUTH_RETRY_INTERVAL_MS);

// Create DataSender instance to post sensor data
DataSender sender(storage, baseUrl);

// MQTT client for publishing sensor data (constructed but only used when MQTT_ENABLED is true)
MqttClient mqttClient(storage, deviceUuid);

// (timing handled by deep sleep across boots)

// Backoff for failed send attempts while connected (ms)
constexpr unsigned long SEND_RETRY_BACKOFF_MS = 30UL * 1000UL; // 30s
static unsigned long lastSendAttempt = 0;

// Sensor instances created from config
static std::vector<std::unique_ptr<SensorBase>> sensors;

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
  //storage.setWifiCreds("", ""); // clear invalid creds
  portal.start();
  Serial.println("Starting portal due to failed WiFi connection");
}

void setup()
{
    Serial.begin(115200);

    // create sensors from config
    sensors = createSensors();

    // TEST: clear saved WiFi credentials on every boot so portal always starts.
    // Comment out the next line when you no longer want this behavior.
    //storage.setWifiCreds("", "");

    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        portal.start();
    } else {
        Serial.print("IP: "); Serial.println(WiFi.localIP());

        // Print saved authentication token if it exists
        String token = storage.getToken();
        if (token.length() > 0) {
            Serial.print("Auth token: ");
            Serial.println(token);
        } else {
            Serial.println("No auth token saved");
        }
    }
    
    // Link MQTT client to DataSender for MQTT support at runtime
    if (MQTT_ENABLED) {
        sender.setMqttClient(&mqttClient);
    }
    
    //print current mqtt credentials
    MqttCredentials creds;
    if (storage.getMqttCredentials(creds)) {
        Serial.println("Saved MQTT credentials:");
        Serial.print("  Server: "); Serial.println(creds.server);
        Serial.print("  Username: "); Serial.println(creds.username);
        Serial.print("  Password: "); Serial.println(creds.password);
    } else {
        Serial.println("No saved MQTT credentials");
    }
    // storage.clearMqttCredentials(); // TESTING: always clear MQTT creds on boot
}

// New helper: read sensors and send measurements
static bool sendMeasurements() {
    // Read sensors into a vector (no manual malloc/free)
    std::vector<SensorReading> readings;
    readings.reserve(sensors.size());

    for (size_t i = 0; i < sensors.size(); ++i) {
        float value;
        if (sensors[i]->read(value)) {
            float rounded = roundf(value * 100.0f) / 100.0f; // 2 decimal places
            readings.push_back({ sensors[i]->uuid(), rounded });
            Serial.print("Sensor ");
            Serial.print(sensors[i]->uuid());
            Serial.print(" = ");
            Serial.println(rounded);
        } else {
            Serial.print("Failed to read from sensor ");
            Serial.println(sensors[i]->uuid());
        }
    }

    if (readings.empty()) {
        return false;
    }

    bool ok = sender.sendReadings(readings.data(), readings.size());
    Serial.print("Data send result: "); Serial.println(ok ? "OK" : "FAILED");
    return ok;
}

void loop()
{
    portal.handle();

    // Only run auth and send measurements when connected to Wi‑Fi
    if (WiFi.status() != WL_CONNECTED) {
        // Not connected: skip auth.loop() and sendMeasurements()
        //Serial.println("WiFi not connected, skipping auth and send");
        return;
    }


    // Let auth manager handle periodic auth attempts when needed
    auth.loop();
    
    // Fetch MQTT credentials if needed and process MQTT events at runtime
    if (MQTT_ENABLED) {
        if (!auth.hasMqttCredentials()) {
            if (auth.getSavedToken().length() > 0) {
                auth.fetchMqttCredentials();
            }
        }
        // Process MQTT events (keep connection alive, handle callbacks)
        mqttClient.loop();
    }

    // Attempt send immediately when connected; device will deep-sleep on success.
    // Rate-limit attempts when sends fail to avoid hammering the server.
    unsigned long now = millis();
    if (now - lastSendAttempt < SEND_RETRY_BACKOFF_MS) {
        // Wait before trying again
        return;
    }

    // Record this attempt time; on failure this enforces the backoff interval.
    lastSendAttempt = now;

    // Call extracted function
    bool sent = sendMeasurements();
    if (sent) {
        // reset lastSendAttempt to avoid delaying next cycle after wake
        lastSendAttempt = 0;
        // Enter deep sleep for the configured interval (milliseconds -> microseconds)
        uint64_t sleep_us = (uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL;
        Serial.print("Measurements sent, entering deep sleep for ms: ");
        Serial.println(SENSORS_READ_INTERVAL_MS);

        // Turn off WiFi cleanly to speed shutdown
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(50);

        esp_sleep_enable_timer_wakeup(sleep_us);
        esp_deep_sleep_start();
    }
}
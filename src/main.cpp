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

// Button pin definition
constexpr int BUTTON_PIN = 14; // GPIO 14 for button

// Replace global Preferences with Storage instance
Storage storage;

// Runtime configuration variables (loaded from storage in setup)
String baseUrl;
bool mqttEnabled;
unsigned long readIntervalMs;

// These will be constructed after loading config
WifiPortal* portal = nullptr;
AuthManager* auth = nullptr;
DataSender* sender = nullptr;
MqttClient* mqttClient = nullptr;

// (timing handled by deep sleep across boots)

// Backoff for failed send attempts while connected (ms)
constexpr unsigned long SEND_RETRY_BACKOFF_MS = 10UL * 1000UL; // 30s
static unsigned long lastSendAttempt = 0;

// Sensor instances created from config
static std::vector<std::unique_ptr<SensorBase>> sensors;

// Forward declaration for one-time provisioning function
static void oneTimeProvisioning();

// Check if button is held for more than 10 seconds to reset all storage
static void checkButtonReset() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button pressed, checking for long press...");
        unsigned long pressStart = millis();
        
        while (digitalRead(BUTTON_PIN) == LOW) {
            if (millis() - pressStart >= BUTTON_LONG_PRESS_MS) {
                Serial.println("Long press detected! Wiping all storage data...");
                storage.clearAll();
                Serial.println("Storage cleared. Restarting device...");
                delay(1000);
                ESP.restart();
            }
            delay(100);
        }
        Serial.println("Button released before 10 seconds");
    }
}

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
  Serial.println("Starting portal due to failed WiFi connection");
}

void setup()
{
    Serial.begin(115200);

    // Button pin setup
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Check for long press to reset storage
    checkButtonReset();

    // Initialize storage defaults from config.h
    DeviceConfig defaults = {
        .baseUrl = BASE_URL,
        .readIntervalMs = SENSORS_READ_INTERVAL_MS,
        .mqttEnabled = MQTT_ENABLED
    };
    storage.loadDefaults(defaults);

    // Load device configuration from storage (uses defaults if not set)
    baseUrl = storage.getBaseUrl();
    readIntervalMs = storage.getReadIntervalMs();
    mqttEnabled = storage.getMqttEnabled();
    
    Serial.println("Device Configuration:");
    Serial.print("  Base URL: "); Serial.println(baseUrl);
    Serial.print("  MQTT Enabled: "); Serial.println(mqttEnabled ? "Yes" : "No");
    Serial.print("  Read Interval: "); Serial.print(readIntervalMs / 1000); Serial.println(" seconds");

    // Now construct objects with loaded configuration
    portal = new WifiPortal(storage, apSsid, apPass, DEFAULT_UUID, DEFAULT_SECRET, 
                           SENSOR_CONFIGS, SENSOR_CONFIG_COUNT, 
                           baseUrl.c_str(), mqttEnabled, readIntervalMs);
    auth = new AuthManager(storage, baseUrl.c_str(), DEFAULT_UUID, DEFAULT_SECRET, AUTH_RETRY_INTERVAL_MS);
    sender = new DataSender(storage, baseUrl.c_str());
    mqttClient = new MqttClient(storage, DEFAULT_UUID);

    // create sensors from config
    sensors = createSensors();


    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        portal->start();
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
    // Perform one-time provisioning (auth/mqtt credentials/connect)
    oneTimeProvisioning();
    
    // Link MQTT client to DataSender for MQTT support at runtime
    if (mqttEnabled) {
        sender->setMqttClient(mqttClient);
    }
    
    //print current mqtt credentials
    MqttCredentials creds;
    if (storage.getMqttCredentials(creds)) {
        // Serial.println("Saved MQTT credentials:");
        // Serial.print("  Server: "); Serial.println(creds.server);
        // Serial.print("  Username: "); Serial.println(creds.username);
        // Serial.print("  Password: "); Serial.println(creds.password);
    } else {
        // Serial.println("No saved MQTT credentials");
    }
    // storage.clearMqttCredentials(); // TESTING: always clear MQTT creds on boot
}

// One-time provisioning: perform auth, fetch MQTT credentials and attempt initial MQTT connect
static void oneTimeProvisioning() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Ensure we have a JWT token (try once synchronously)
    String token = storage.getToken();
    if (token.length() == 0) {
        Serial.println("No token saved, attempting immediate authentication...");
        auth->tryAuthenticateOnce();
        token = storage.getToken();
        if (token.length() > 0) Serial.println("Authentication succeeded and token was saved");
    }

    if (!mqttEnabled) return;

    // If we don't have MQTT credentials but we do have a token, fetch them once
    if (!auth->hasMqttCredentials() && token.length() > 0) {
        Serial.println("Fetching MQTT credentials (one-time)");
        if (auth->fetchMqttCredentials()) {
            Serial.println("MQTT credentials fetched and stored");
        } else {
            Serial.println("Failed to fetch MQTT credentials in setup");
        }
    }

    // Try a single MQTT connect attempt if credentials are available
    if (auth->hasMqttCredentials()) {
        Serial.println("Attempting initial MQTT connect from setup");
        if (mqttClient->connect()) {
            Serial.println("Initial MQTT connect succeeded");
        } else {
            Serial.println("Initial MQTT connect failed (will retry in loop)");
        }
    }
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

    bool ok = sender->sendReadings(readings.data(), readings.size());
    Serial.print("Data send result: "); Serial.println(ok ? "OK" : "FAILED");
    return ok;
}

void loop()
{
    portal->handle();

    // Only run auth and send measurements when connected to Wi‑Fi
    if (WiFi.status() != WL_CONNECTED) {
        // Not connected: skip auth.loop() and sendMeasurements()
        //Serial.println("WiFi not connected, skipping auth and send");
        return;
    }


    // Let auth manager handle periodic auth attempts when needed
    auth->loop();
    
    // MQTT runtime processing disabled; initial connect happens in setup() only.

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
        uint64_t sleep_us = (uint64_t)readIntervalMs * 1000ULL;
        Serial.print("Measurements sent, entering deep sleep for ms: ");
        Serial.println(readIntervalMs);

        // Turn off WiFi cleanly to speed shutdown
        if (mqttEnabled) {
            mqttClient->disconnect();
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(50);

        // Enable wakeup from button (GPIO 14, LOW = pressed)
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);
        // Also enable timer wakeup
        esp_sleep_enable_timer_wakeup(sleep_us);
        esp_deep_sleep_start();
    }
}
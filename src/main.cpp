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

// non-blocking read interval
static unsigned long lastDhtRead = 0;

// Sensor instances created from config
static SensorBase** sensors = nullptr;
static size_t sensorsCount = 0;

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

void setup()
{
    Serial.begin(115200);

    // create sensors from config
    sensors = createSensors(sensorsCount);

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
}

void loop()
{
    portal.handle();

    // Let auth manager handle periodic auth attempts when needed
    auth.loop();

    // Read sensors from the abstraction layer
    size_t maxReadings = sensorsCount;
    SensorReading *readings = (SensorReading*)malloc(sizeof(SensorReading) * maxReadings);
    size_t available = 0;

    for (size_t i = 0; i < sensorsCount; ++i) {
        float value;
        if (sensors[i]->read(value)) {
            float rounded = roundf(value * 100.0f) / 100.0f; // 2 decimal places
            readings[available++] = { sensors[i]->uuid(), rounded };
            Serial.print("Sensor ");
            Serial.print(sensors[i]->uuid());
            Serial.print(" = ");
            Serial.println(rounded);
        } else {
            Serial.print("Failed to read from sensor ");
            Serial.println(sensors[i]->uuid());
        }
    }

    if (available > 0) {
        bool ok = sender.sendReadings(readings, available);
        Serial.print("Data send result: "); Serial.println(ok ? "OK" : "FAILED");
    }

    free(readings);

    // Wait a bit before scanning again
    delay(SENSORS_READ_INTERVAL_MS);
}
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
#include <DHT.h>                      // add DHT include
#include "config.h"
#include "data_sender.h"
#include <math.h>

const char* apSsid = AP_SSID;
const char* apPass = AP_PASS; // optional

// Replace global Preferences with Storage instance
Storage storage;
WifiPortal portal(storage, apSsid, apPass);

const char* baseUrl = BASE_URL; // e.g. "http://example.com" (no trailing slash)
const char* deviceUuid = DEFAULT_UUID;
const char* deviceSecret = DEFAULT_SECRET;

// DHT pin and read interval are defined in include/config.h (DHT11_PIN, DHT_READ_INTERVAL_MS)

// Auth manager
AuthManager auth(storage, baseUrl, deviceUuid, deviceSecret, AUTH_RETRY_INTERVAL_MS);

// Create DataSender instance to post sensor data
DataSender sender(storage, baseUrl);



DHT dht11(DHT11_PIN, DHT11);

// non-blocking read interval
static unsigned long lastDhtRead = 0;

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

    // init DHT
    dht11.begin(); // initialize the DHT11 sensor

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

    float humi  = dht11.readHumidity();
    // read temperature in Celsius
    float tempC = dht11.readTemperature();
    // read temperature in Fahrenheit
    float tempF = dht11.readTemperature(true);

    // check whether the reading is successful or not
    if ( isnan(tempC) || isnan(tempF) || isnan(humi)) {
      Serial.println("Failed to read from DHT11 sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humi);
      Serial.print("%");

      Serial.print("  |  ");

      Serial.print("Temperature: ");
      Serial.print(tempC);
      Serial.print("°C  ~  ");
      Serial.print(tempF);
      Serial.println("°F");

      // Build explicit readings array (uuid + value) and send
      float tempC_rounded = roundf(tempC * 100.0f) / 100.0f; // 2 decimal places
      float humi_rounded  = roundf(humi  * 100.0f) / 100.0f; // 2 decimal places
      SensorReading readings[] = {
          { SENSOR_UUIDS[0], tempC_rounded },
          { SENSOR_UUIDS[1], humi_rounded }
      };
      bool ok = sender.sendReadings(readings, sizeof(readings)/sizeof(readings[0]));
      Serial.print("Data send result: "); Serial.println(ok ? "OK" : "FAILED");
    }

    // Wait a bit before scanning again
    delay(100000);
}
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

const char* apSsid = "ESP_Config";
const char* apPass = ""; // optional

// Replace global Preferences with Storage instance
Storage storage;
WifiPortal portal(storage, apSsid, apPass);

const char* baseUrl = "https://agronos.kalogeitonas.xyz"; // e.g. "http://example.com" (no trailing slash)
const char* deviceUuid = "Test-Device-1";
const char* deviceSecret = "Test-Device-1";

// Auth manager
AuthManager auth(storage, baseUrl, deviceUuid, deviceSecret, 30000);

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

    // TEST: clear saved WiFi credentials on every boot so portal always starts.
    // Comment out the next line when you no longer want this behavior.
    //storage.setWifiCreds("", "");

    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        portal.start();
    } else {
        Serial.print("IP: "); Serial.println(WiFi.localIP());
        // Attempt authentication once after successful connection
        bool authOk = auth.tryAuthenticateOnce();
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

    // Let auth manager handle periodic auth attempts when needed
    auth.loop();

     // Wait a bit before scanning again
     delay(5000);
}
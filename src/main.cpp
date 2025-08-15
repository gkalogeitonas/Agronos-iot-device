/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>

const char* apSsid = "ESP_Config";
const char* apPass = ""; // optional
IPAddress apIP(192,168,4,1);

DNSServer dnsServer;
WebServer webServer(80);
Preferences prefs;

bool portalRunning = false;

const char* baseUrl = "https://agronos.kalogeitonas.xyz"; // e.g. "http://example.com" (no trailing slash)
const char* deviceUuid = "Test-Device-1";
const char* deviceSecret = "Test-Device-1";

String indexPage = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
<body><h3>Configure WiFi</h3>
<form action="/save" method="POST">
SSID:<br><input name="ssid"><br>Password:<br><input name="pass" type="password"><br><br>
<input type="submit" value="Save">
</form></body></html>
)rawliteral";

void handleRoot() {
  webServer.send(200, "text/html", indexPage);
}

void handleSave() {
  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");
  if (ssid.length() > 0) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    webServer.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
    delay(1000);
    ESP.restart();
  } else {
    webServer.send(400, "text/plain", "SSID required");
  }
}

void startPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(apSsid, apPass);
  dnsServer.start(53, "*", apIP);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.begin();
  portalRunning = true;
  Serial.println("AP started, captive portal running");
}

void tryAutoConnect() {
  // Open non-readonly so the namespace will be created if it doesn't exist
  prefs.begin("wifi", false);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();
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
  prefs.begin("auth", false);
  prefs.putString("token", token);
  prefs.end();
  Serial.print("Saved auth token: "); Serial.println(token);
}

String getSavedAuthToken() {
  // Open non-readonly so the namespace will be created if it doesn't exist
  prefs.begin("auth", false);
  String t = prefs.getString("token", "");
  prefs.end();
  return t;
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
    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        startPortal();
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
    // Run captive-portal handlers only while portal is active
    if (portalRunning) {
        dnsServer.processNextRequest();
        webServer.handleClient();

        // If the device becomes connected while portal is running, stop the portal
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected while portal running — stopping portal");
            WiFi.softAPdisconnect(true);
            dnsServer.stop();
            webServer.stop();
            portalRunning = false;
        }
    }

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
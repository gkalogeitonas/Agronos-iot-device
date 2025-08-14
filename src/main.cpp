/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

const char* apSsid = "ESP_Config";
const char* apPass = ""; // optional
IPAddress apIP(192,168,4,1);

DNSServer dnsServer;
WebServer webServer(80);
Preferences prefs;

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
  Serial.println("AP started, captive portal running");
}

void tryAutoConnect() {
  prefs.begin("wifi", true);
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

void setup()
{
    Serial.begin(115200);
    tryAutoConnect();
    if (WiFi.status() != WL_CONNECTED) {
        startPortal();
    } else {
        Serial.print("IP: "); Serial.println(WiFi.localIP());
    }
}

void loop()
{
    dnsServer.processNextRequest();
    webServer.handleClient();

    Serial.println("scan start");
    Serial.println("Current WiFi status: ");
    Serial.println(WiFi.status());
    Serial.println("Current IP address: ");
    Serial.println(WiFi.localIP());

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    delay(5000);
}
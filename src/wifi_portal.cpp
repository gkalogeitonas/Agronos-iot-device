#include "wifi_portal.h"
#include <WiFi.h>

WifiPortal::WifiPortal(Storage &storage, const char* apSsid, const char* apPass)
: storage(storage), webServer(80), ssid(apSsid), pass(apPass), apIP(192,168,4,1), running(false)
{
  indexPage = R"rawliteral(
  <!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
  <body><h3>Configure WiFi</h3>
  <form action="/save" method="POST">
  SSID:<br><input name="ssid"><br>Password:<br><input name="pass" type="password"><br><br>
  <input type="submit" value="Save">
  </form></body></html>
  )rawliteral";
}

void WifiPortal::onRoot() {
  webServer.send(200, "text/html", indexPage);
}

void WifiPortal::onSave() {
  String ssidArg = webServer.arg("ssid");
  String passArg = webServer.arg("pass");
  if (ssidArg.length() > 0) {
    storage.setWifiCreds(ssidArg, passArg);
    webServer.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
    delay(1000);
    ESP.restart();
  } else {
    webServer.send(400, "text/plain", "SSID required");
  }
}

void WifiPortal::start() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(ssid, pass);
  dnsServer.start(53, "*", apIP);
  // Main portal page and save endpoint
  webServer.on("/", HTTP_GET, std::bind(&WifiPortal::onRoot, this));
  webServer.on("/save", HTTP_POST, std::bind(&WifiPortal::onSave, this));

  // Common captive-portal checks used by Android / iOS / Windows / macOS
  // Android uses /generate_204 (expecting a 204 No Content)
  webServer.on("/generate_204", HTTP_GET, [this]() {
    webServer.send(204, "text/plain", "");
  });
  // Some platforms query for a small file or redirect; reply with the portal page
  webServer.on("/hotspot-detect.html", HTTP_GET, std::bind(&WifiPortal::onRoot, this));
  webServer.on("/ncsi.txt", HTTP_GET, std::bind(&WifiPortal::onRoot, this));
  webServer.on("/connectivity-check.html", HTTP_GET, std::bind(&WifiPortal::onRoot, this));

  // Favicon requests (silence them)
  webServer.on("/favicon.ico", HTTP_GET, [this]() {
    webServer.send(204, "image/x-icon", "");
  });

  // Fallback: serve the portal page for any unknown request (prevents the WebServer "not found" log)
  webServer.onNotFound(std::bind(&WifiPortal::onRoot, this));
  webServer.begin();
  running = true;
  Serial.println("AP started, captive portal running");
}

void WifiPortal::stop() {
  if (!running) return;
  WiFi.softAPdisconnect(true);
  dnsServer.stop();
  webServer.stop();
  running = false;
  Serial.println("Portal stopped");
}

void WifiPortal::handle() {
  if (!running) return;
  dnsServer.processNextRequest();
  webServer.handleClient();
  // If the device becomes connected while portal is running, stop the portal
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected while portal running — stopping portal");
    stop();
  }
}

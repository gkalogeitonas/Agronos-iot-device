#include "wifi_portal.h"
#include <WiFi.h>

WifiPortal::WifiPortal(Storage &storage, const char* apSsid, const char* apPass, 
                       const char* deviceUuid, const char* deviceSecret, const SensorConfig* sensorConfigs, size_t sensorCount,
                       const char* baseUrl, bool mqttEnabled, unsigned long readIntervalMs)
: storage(storage), webServer(80), ssid(apSsid), pass(apPass), apIP(192,168,4,1), running(false),
  deviceUuid(deviceUuid), deviceSecret(deviceSecret), sensorConfigs(sensorConfigs), sensorCount(sensorCount),
  baseUrl(baseUrl), mqttEnabled(mqttEnabled), readIntervalMs(readIntervalMs)
{
  // indexPage will be generated dynamically in generateHtmlPage()
}

void WifiPortal::onRoot() {
  String html = generateHtmlPage();
  webServer.send(200, "text/html", html);
}

void WifiPortal::scanNetworks() {
  availableNetworks.clear();
  Serial.println("Scanning for available WiFi networks...");
  
  // Perform WiFi scan (set to async=true to non-blocking)
  int numNetworks = WiFi.scanNetworks(false, false);
  
  if (numNetworks == 0) {
    Serial.println("No networks found");
    return;
  }
  
  Serial.print("Found ");
  Serial.print(numNetworks);
  Serial.println(" networks");
  
  // Collect unique network names (remove duplicates)
  for (int i = 0; i < numNetworks; ++i) {
    String networkName = WiFi.SSID(i);
    
    // Skip empty SSIDs
    if (networkName.length() == 0) continue;
    
    // Check if we already have this network in our list
    bool isDuplicate = false;
    for (const auto& existing : availableNetworks) {
      if (existing == networkName) {
        isDuplicate = true;
        break;
      }
    }
    
    if (!isDuplicate) {
      availableNetworks.push_back(networkName);
      Serial.print("  - ");
      Serial.print(networkName);
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(")");
    }
  }
  
  WiFi.scanDelete(); // Free memory used by scan
}

String WifiPortal::generateHtmlPage() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
      h3 { color: #333; }
      .device-info { background-color: #e8f4f8; padding: 15px; border-radius: 4px; margin-bottom: 20px; }
      .device-info h4 { margin-top: 0; color: #1976d2; }
      .device-uuid { font-size: 12px; color: #666; word-break: break-all; margin: 5px 0; }
      .sensors-list { margin-left: 15px; font-size: 13px; }
      .sensor-item { margin: 5px 0; color: #555; }
      form { margin-top: 20px; background-color: white; padding: 20px; border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
      label { display: block; margin-bottom: 5px; font-weight: bold; }
      select, input[type="password"] { width: 100%; max-width: 300px; padding: 8px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
      input[type="submit"] { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold; }
      input[type="submit"]:hover { background-color: #45a049; }
      .info { color: #666; font-size: 14px; margin-bottom: 10px; }
    </style>
  </head>
  <body>
    <div class="device-info">
      <h4>Device Information</h4>
      <div>
        <strong>Device UUID:</strong>
        <div class="device-uuid">)rawliteral";
  
  html += deviceUuid ? deviceUuid : "Not configured";
  
  html += R"rawliteral(</div>
      </div>
      
      <div style="margin-top: 10px;">
        <strong>Device Secret:</strong>
        <div class="device-uuid">)rawliteral";
  
  html += deviceSecret ? deviceSecret : "Not configured";
  
  html += R"rawliteral(</div>
      </div>
      
      <div style="margin-top: 15px;">
        <strong>Sensors:</strong>
        <div class="sensors-list">)rawliteral";
  
  // Add sensor information
  if (sensorConfigs && sensorCount > 0) {
    for (size_t i = 0; i < sensorCount; ++i) {
      html += "<div class=\"sensor-item\">";
      html += sensorConfigs[i].displayName;
      html += " (UUID: ";
      html += sensorConfigs[i].uuid;
      html += ")</div>";
    }
  } else {
    html += "<div class=\"sensor-item\">No sensors configured</div>";
  }
  
  html += R"rawliteral(
        </div>
      </div>
    </div>

    <h3>Configure WiFi</h3>
    <p class="info">Select your WiFi network and enter the password</p>
    <form action="/save" method="POST">
      <label for="ssid">WiFi Network:</label>
      <select name="ssid" id="ssid" required>
        <option value="">-- Select a network --</option>
  )rawliteral";
  
  // Add available networks to the dropdown
  for (const auto& network : availableNetworks) {
    html += "        <option value=\"";
    html += network;
    html += "\">";
    html += network;
    html += "</option>\n";
  }
  
  html += R"rawliteral(
      </select>
      <label for="pass">Password:</label>
      <input type="password" name="pass" id="pass" placeholder="Enter WiFi password">
      
      <h3 style="margin-top: 30px; color: #333;">Device Configuration</h3>
      <p class="info">Configure device settings (optional)</p>
      
      <label for="base_url">Server URL:</label>
      <input type="text" name="base_url" id="base_url" placeholder="https://example.com" value=")rawliteral";
  
  // Pre-populate base URL
  html += baseUrl ? baseUrl : "";
  
  html += R"rawliteral(">
      
      <label for="read_interval_minutes">Read Interval (minutes):</label>
      <input type="number" name="read_interval_minutes" id="read_interval_minutes" min="1" value=")rawliteral";
  
  // Pre-populate read interval in minutes
  html += String(readIntervalMs / 60000);
  
  html += R"rawliteral(">
      
      <label>
        <input type="checkbox" name="mqtt_enabled" id="mqtt_enabled" value="on")rawliteral";
  
  // Pre-populate MQTT enabled checkbox
  if (mqttEnabled) {
    html += " checked";
  }
  
  html += R"rawliteral(>
        Enable MQTT protocol (uncheck to use HTTP only)
      </label>
      
      <br><br>
      <input type="submit" value="Save & Connect">
    </form>
  </body>
  </html>
  )rawliteral";
  
  return html;
}

void WifiPortal::onSave() {
  String ssidArg = webServer.arg("ssid");
  String passArg = webServer.arg("pass");
  
  // Extract device config parameters
  String baseUrlArg = webServer.arg("base_url");
  String readIntervalArg = webServer.arg("read_interval_minutes");
  bool mqttEnabledArg = webServer.hasArg("mqtt_enabled");
  
  if (ssidArg.length() > 0) {
    // Save WiFi credentials
    storage.setWifiCreds(ssidArg, passArg);
    
    // Save device configuration if provided
    if (baseUrlArg.length() > 0) {
      storage.setBaseUrl(baseUrlArg);
    }
    
    if (readIntervalArg.length() > 0) {
      unsigned long intervalMinutes = readIntervalArg.toInt();
      unsigned long intervalMs = intervalMinutes * 60 * 1000;
      storage.setReadIntervalMs(intervalMs);
    }
    
    // Save MQTT enabled flag (checkbox returns "on" when checked, absent when unchecked)
    storage.setMqttEnabled(mqttEnabledArg);
    
    webServer.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
    delay(5000);
    ESP.restart();
  } else {
    webServer.send(400, "text/plain", "SSID required");
  }
}

void WifiPortal::start() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(ssid, pass);
  
  // Scan for available networks before starting the server
  scanNetworks();
  
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

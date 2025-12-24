#include "wifi_portal.h"
#include <WiFi.h>

WifiPortal::WifiPortal(Storage &storage, const char* apSsid, const char* apPass)
: storage(storage), webServer(80), ssid(apSsid), pass(apPass), apIP(192,168,4,1), running(false)
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
      body { font-family: Arial, sans-serif; margin: 20px; }
      h3 { color: #333; }
      form { margin-top: 20px; }
      label { display: block; margin-bottom: 5px; font-weight: bold; }
      select, input[type="password"] { width: 100%; max-width: 300px; padding: 8px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px; }
      input[type="submit"] { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }
      input[type="submit"]:hover { background-color: #45a049; }
      .info { color: #666; font-size: 14px; margin-bottom: 10px; }
    </style>
  </head>
  <body>
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
      <br><br>
      <input type="submit" value="Connect">
    </form>
  </body>
  </html>
  )rawliteral";
  
  return html;
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

#include "storage.h"

Storage::Storage() {
  // nothing
}

bool Storage::getWifiCreds(String &ssid, String &pass) {
  prefs.begin("wifi", false);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  return ssid.length() > 0;
}

void Storage::setWifiCreds(const String &ssid, const String &pass) {
  prefs.begin("wifi", false);
  String oldS = prefs.getString("ssid", "");
  String oldP = prefs.getString("pass", "");
  if (ssid != oldS || pass != oldP) {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
  }
  prefs.end();
}

String Storage::getToken() {
  prefs.begin("auth", false);
  String t = prefs.getString("token", "");
  prefs.end();
  return t;
}

void Storage::setToken(const String &token) {
  prefs.begin("auth", false);
  String old = prefs.getString("token", "");
  if (token != old) prefs.putString("token", token);
  prefs.end();
}

bool Storage::getMqttCredentials(MqttCredentials &creds) {
  prefs.begin("mqtt", false);
  creds.server = prefs.getString("server", "");
  creds.username = prefs.getString("username", "");
  creds.password = prefs.getString("password", "");
  prefs.end();
  
  creds.isValid = (creds.server.length() > 0 && 
                   creds.username.length() > 0 && 
                   creds.password.length() > 0);
  return creds.isValid;
}

void Storage::setMqttCredentials(const String &server, const String &username, const String &password) {
  prefs.begin("mqtt", false);
  prefs.putString("server", server);
  prefs.putString("username", username);
  prefs.putString("password", password);
  prefs.end();
  Serial.println("MQTT credentials saved to storage");
}

void Storage::clearMqttCredentials() {
  prefs.begin("mqtt", false);
  prefs.clear();
  prefs.end();
  Serial.println("MQTT credentials cleared");
}

bool Storage::hasMqttCredentials() {
  MqttCredentials creds;
  return getMqttCredentials(creds);
}

// ==================== Device Configuration (Config Struct Pattern) ====================

Storage* Storage::loadDefaults(const DeviceConfig& defaults) {
  _defaults = defaults;
  return this;
}

void Storage::ensureConfigLoaded() {
  if (_configLoaded) return;
  
  prefs.begin("config", true); // read-only mode
  
  // Load all config values with defaults as fallback
  _cache.baseUrl = prefs.getString("base_url", _defaults.baseUrl);
  _cache.readIntervalMs = prefs.getULong("interval_ms", _defaults.readIntervalMs);
  
  // Handle mqttEnabled with isKey check for proper default
  if (prefs.isKey("mqtt_enabled")) {
    _cache.mqttEnabled = prefs.getBool("mqtt_enabled", _defaults.mqttEnabled);
  } else {
    _cache.mqttEnabled = _defaults.mqttEnabled;
  }
  
  prefs.end();
  _configLoaded = true;
}

String Storage::getBaseUrl() {
  ensureConfigLoaded();
  return _cache.baseUrl;
}

unsigned long Storage::getReadIntervalMs() {
  ensureConfigLoaded();
  return _cache.readIntervalMs;
}

bool Storage::getMqttEnabled() {
  ensureConfigLoaded();
  return _cache.mqttEnabled;
}

void Storage::saveConfig(const DeviceConfig& cfg) {
  ensureConfigLoaded(); // Ensure cache is populated
  
  prefs.begin("config", false);
  
  // Only write changed fields to prevent Flash wear
  if (cfg.baseUrl != _cache.baseUrl) {
    prefs.putString("base_url", cfg.baseUrl);
  }
  
  if (cfg.readIntervalMs != _cache.readIntervalMs) {
    prefs.putULong("interval_ms", cfg.readIntervalMs);
  }
  
  if (cfg.mqttEnabled != _cache.mqttEnabled) {
    prefs.putBool("mqtt_enabled", cfg.mqttEnabled);
  }
  
  prefs.end();
  
  // Update cache to match new values
  _cache = cfg;
}

void Storage::setBaseUrl(const String &url) {
  DeviceConfig cfg = _cache;
  cfg.baseUrl = url;
  saveConfig(cfg);
}

void Storage::setReadIntervalMs(unsigned long ms) {
  DeviceConfig cfg = _cache;
  cfg.readIntervalMs = ms;
  saveConfig(cfg);
}

void Storage::setMqttEnabled(bool enabled) {
  DeviceConfig cfg = _cache;
  cfg.mqttEnabled = enabled;
  saveConfig(cfg);
}

void Storage::clearAll() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  prefs.begin("auth", false);
  prefs.clear();
  prefs.end();
  prefs.begin("mqtt", false);
  prefs.clear();
  prefs.end();
  prefs.begin("config", false);
  prefs.clear();
  prefs.end();
  
  // Invalidate cache since NVS was cleared
  _configLoaded = false;
}

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

String Storage::getBaseUrl() {
  prefs.begin("config", false);
  String url = prefs.getString("base_url", "");
  prefs.end();
  return url;
}

void Storage::setBaseUrl(const String &url) {
  prefs.begin("config", false);
  String old = prefs.getString("base_url", "");
  if (url != old) prefs.putString("base_url", url);
  prefs.end();
}

bool Storage::getMqttEnabled() {
  prefs.begin("config", false);
  // Use getBool with no default to detect if key exists
  // If key doesn't exist, isKey returns false
  bool enabled = true; // default value
  if (prefs.isKey("mqtt_enabled")) {
    enabled = prefs.getBool("mqtt_enabled", true);
  }
  prefs.end();
  return enabled;
}

void Storage::setMqttEnabled(bool enabled) {
  prefs.begin("config", false);
  bool old = prefs.getBool("mqtt_enabled", true);
  if (enabled != old) prefs.putBool("mqtt_enabled", enabled);
  prefs.end();
}

unsigned long Storage::getReadIntervalMs() {
  prefs.begin("config", false);
  unsigned long interval = prefs.getULong("interval_ms", 0);
  prefs.end();
  return interval;
}

void Storage::setReadIntervalMs(unsigned long ms) {
  prefs.begin("config", false);
  unsigned long old = prefs.getULong("interval_ms", 0);
  if (ms != old) prefs.putULong("interval_ms", ms);
  prefs.end();
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
}

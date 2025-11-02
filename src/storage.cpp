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
}

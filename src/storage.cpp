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

void Storage::clearAll() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  prefs.begin("auth", false);
  prefs.clear();
  prefs.end();
}

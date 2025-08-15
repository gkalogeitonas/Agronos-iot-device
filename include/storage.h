#pragma once
#include <Preferences.h>
#include <Arduino.h>

class Storage {
public:
  Storage();

  // Wifi credentials
  // Returns true if credentials exist
  bool getWifiCreds(String &ssid, String &pass);
  void setWifiCreds(const String &ssid, const String &pass);

  // Auth token
  String getToken();
  void setToken(const String &token);

  // Clear stored credentials and token
  void clearAll();

private:
  Preferences prefs;
};

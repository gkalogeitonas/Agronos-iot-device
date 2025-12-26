#pragma once
#include <Preferences.h>
#include <Arduino.h>

struct MqttCredentials {
    String server;
    String username;
    String password;
    bool isValid;
};

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

  // MQTT credentials
  bool getMqttCredentials(MqttCredentials &creds);
  void setMqttCredentials(const String &server, const String &username, const String &password);
  void clearMqttCredentials();
  bool hasMqttCredentials();

  // Device configuration
  String getBaseUrl();
  void setBaseUrl(const String &url);
  bool getMqttEnabled();
  void setMqttEnabled(bool enabled);
  unsigned long getReadIntervalMs();
  void setReadIntervalMs(unsigned long ms);

  // Clear stored credentials and token
  void clearAll();

private:
  Preferences prefs;
};

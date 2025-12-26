#pragma once
#include <Preferences.h>
#include <Arduino.h>

struct MqttCredentials {
    String server;
    String username;
    String password;
    bool isValid;
};

struct DeviceConfig {
    String baseUrl;
    unsigned long readIntervalMs;
    bool mqttEnabled;
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

  // Device configuration - Dependency Injection
  Storage* loadDefaults(const DeviceConfig& defaults);

  // Device configuration - Transparent Getters
  String getBaseUrl();
  unsigned long getReadIntervalMs();
  bool getMqttEnabled();

  // Device configuration - Individual Setters
  void setBaseUrl(const String &url);
  void setReadIntervalMs(unsigned long ms);
  void setMqttEnabled(bool enabled);

  // Device configuration - Atomic Setter
  void saveConfig(const DeviceConfig& cfg);

  // Clear stored credentials and token
  void clearAll();

private:
  Preferences prefs;
  
  // Config cache and defaults
  DeviceConfig _cache;
  DeviceConfig _defaults;
  bool _configLoaded = false;
  
  // Lazy-loading helper
  void ensureConfigLoaded();
};

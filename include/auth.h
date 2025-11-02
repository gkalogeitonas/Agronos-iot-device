#pragma once
#include <Arduino.h>
#include "storage.h"

class AuthManager {
public:
  AuthManager(Storage &storage, const char* baseUrl, const char* uuid, const char* secret, unsigned long retryIntervalMs = 30000);
  void begin();
  // Try authenticate once immediately (blocking network call)
  bool tryAuthenticateOnce();
  // Call from main loop to perform periodic attempts when needed
  void loop();
  String getSavedToken();
  
  // MQTT credential provisioning
  bool fetchMqttCredentials();
  bool hasMqttCredentials();

private:
  Storage &storage;
  const char* baseUrl;
  const char* uuid;
  const char* secret;
  unsigned long retryIntervalMs;
  unsigned long lastAttempt;

  bool performAuthRequest();
  bool performMqttCredentialsRequest();
};

#pragma once
#include <DNSServer.h>
#include <WebServer.h>
#include "storage.h"
#include "sensor.h"
#include <Arduino.h>
#include <vector>

class WifiPortal {
public:
  WifiPortal(Storage &storage, const char* apSsid = "ESP_Config", const char* apPass = "", 
             const char* deviceUuid = "", const char* deviceSecret = "", const SensorConfig* sensorConfigs = nullptr, size_t sensorCount = 0,
             const char* baseUrl = "", bool mqttEnabled = true, unsigned long readIntervalMs = 120000);
  void start();
  void stop();
  void handle();
  bool isRunning() const { return running; }

private:
  Storage &storage;
  DNSServer dnsServer;
  WebServer webServer;
  const char* ssid;
  const char* pass;
  IPAddress apIP;
  bool running;
  String indexPage;
  std::vector<String> availableNetworks;
  const char* deviceUuid;
  const char* deviceSecret;
  const SensorConfig* sensorConfigs;
  size_t sensorCount;
  const char* baseUrl;
  bool mqttEnabled;
  unsigned long readIntervalMs;

  void scanNetworks();
  String generateHtmlPage();
  void onRoot();
  void onSave();
};

#pragma once
#include <DNSServer.h>
#include <WebServer.h>
#include "storage.h"
#include <Arduino.h>
#include <vector>

class WifiPortal {
public:
  WifiPortal(Storage &storage, const char* apSsid = "ESP_Config", const char* apPass = "");
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

  void scanNetworks();
  String generateHtmlPage();
  void onRoot();
  void onSave();
};

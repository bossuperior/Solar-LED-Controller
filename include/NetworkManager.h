#pragma once
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "LogManager.h"

class NetworkManager
{
private:
  WiFiMulti wifiMulti;
  wl_status_t lastStatus = WL_IDLE_STATUS;
  unsigned long lastNetCheck = 0;
  const unsigned long netInterval = 30000; // Check Internet every 30 seconds
  bool _hasInternet = false;
  bool _firstCheck = true;
  LogManager* m_logger = nullptr;

public:
  void begin(LogManager* sysLogger);
  void addAP(const char *ssid, const char *pass);
  void handle();
  bool isInternetAvailable();
};
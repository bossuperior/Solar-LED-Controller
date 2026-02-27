#pragma once
#include <WiFiMulti.h>
#include <HTTPClient.h>

class NetworkManager
{
private:
  WiFiMulti wifiMulti;
  wl_status_t lastStatus = WL_IDLE_STATUS;
  unsigned long lastNetCheck = 0;
  const unsigned long netInterval = 30000; // Check Internet every 30 seconds
  bool _hasInternet = false;
  bool _firstCheck = true;

public:
  void begin();
  void addAP(const char *ssid, const char *pass);
  void handle();
  bool isInternetAvailable();
};
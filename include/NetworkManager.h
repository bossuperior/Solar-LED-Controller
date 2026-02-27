#pragma once
#include <WiFiMulti.h>

class NetworkManager
{
private:
  WiFiMulti wifiMulti;
  wl_status_t lastStatus = WL_IDLE_STATUS;
public:
  void begin();
  void addAP(const char* ssid, const char* pass);
  void handle();
};
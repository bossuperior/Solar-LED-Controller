#pragma once
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "LogManager.h"
#include "secret.h"
#include "Configs.h"

class NetworkManager
{
private:
  WiFiMulti wifiMulti;
  wl_status_t lastStatus = WL_IDLE_STATUS;
  LogManager *m_logger = nullptr;
  unsigned long lastNetCheck = 0, _startAttemptTime = 0;
  bool _hasInternet = false, _firstCheck = true, _apModeStarted = false;

public:
  void begin(LogManager *sysLogger);
  void handle();
  bool isInternetAvailable();
};
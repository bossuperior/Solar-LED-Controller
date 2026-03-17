#pragma once
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <Update.h>
#include "LogManager.h"
#include "secret.h"
#include "PowerManager.h"

class OTAManager
{
private:
  const char *_updateUrl;
  LogManager *m_logger;
  PowerManager *m_power;
  bool _isWaitingValidation = false;

public:
  bool isUpdating = false;
  void begin();
  void checkUpdate(String currentVersion,LogManager* sysLogger ,PowerManager* pm, bool force = false);
  void triggerRollback();
  void validateUpdate();
  void handleSafetyTimer();
};
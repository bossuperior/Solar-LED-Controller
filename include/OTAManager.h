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
#include "BlynkManager.h"

class BlynkManager;

class OTAManager
{
private:
  LogManager *m_logger;
  PowerManager *m_power;
  BlynkManager *m_blynk;

public:
  bool isUpdating = false;
  void checkUpdate(String currentVersion,LogManager* sysLogger ,PowerManager* pm, BlynkManager* bk, bool force = false);
  void triggerRollback(BlynkManager* bk);
};
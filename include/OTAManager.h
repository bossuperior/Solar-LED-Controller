#pragma once
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "LogManager.h"
#include "secret.h"

class OTAManager
{
private:
  const char *_updateUrl;
  LogManager *m_logger;

public:
  bool isUpdating = false;
  void checkUpdate(String currentVersion,LogManager* sysLogger);
};
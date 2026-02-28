#pragma once
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

class OTAManager
{
private:
  const char *_updateUrl;

public:
  bool isUpdating = false;
  void checkUpdate(String currentVersion);
};
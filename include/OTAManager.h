#pragma once
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

class OTAManager
{
private:
  const char *_updateUrl;

public:
  bool isUpdating = false;
  void checkUpdate(const char *currentVersion);
};
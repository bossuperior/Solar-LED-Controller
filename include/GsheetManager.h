#pragma once
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "LogManager.h"
#include "secret.h"
#include "TimeManager.h"
#include "Configs.h"

class GsheetManager
{
private:
    LogManager* m_sysLogger = nullptr;
    TimeManager* m_timeManager = nullptr;
    WiFiClientSecure m_client;
public:
    void begin(LogManager* sysLogger, TimeManager* timeManager);
    void sendData(float voltage, float tempBuck, float tempChip, int fanSpeed, const String& lightPct);
};
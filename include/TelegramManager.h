#pragma once
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <Preferences.h>
#include <WiFi.h>
#include "secret.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"

class LightManager;

class TelegramManager
{
private:
    WiFiClientSecure client;
    UniversalTelegramBot *bot;
    String m_token;
    String m_chatId;
    LogManager *m_sysLogger;

public:
    void begin(LogManager *sysLogger);
    void sendAlert(String module, String message);
    void checkMessages(PowerManager *pm, TempManager *tm, FanManager *fm, LightManager *lm);
};
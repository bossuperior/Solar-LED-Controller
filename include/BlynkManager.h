#pragma once
#include <Arduino.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <freertos/semphr.h>
#include "LogManager.h"
#include "LightManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"

class BlynkManager
{
private:
    bool pendingOTA = false;
    String otaDownloadUrl = "";
    String b_fwVer = "";
    LightManager *b_light = nullptr;
    PowerManager *b_power = nullptr;
    TempManager *b_temp = nullptr;
    FanManager *b_fan = nullptr;
    LogManager *b_logger = nullptr;
    TimeManager *b_time = nullptr;
    SemaphoreHandle_t *b_mutex = nullptr;

public:
    void begin(LogManager *logger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, TimeManager *time, SemaphoreHandle_t *mutex, const String &fwVer);
    void handle();
    void sendTelemetry();
    void sendLog(const String &msg);
    void checkOTA();
    void setPendingOTA(String url) {
        otaDownloadUrl = url;
        pendingOTA = true;
    }
};
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
    TempManager *b_temp = nullptr;
    TimeManager *b_time = nullptr;

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
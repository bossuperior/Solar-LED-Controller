#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include "LogManager.h"
#include "LightManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#include "Configs.h"

class OTAManager;
class BlynkManager
{
public:
    void begin(LogManager *logger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, TimeManager *time, SemaphoreHandle_t *mutex, OTAManager *ota, const String &fwVer);
    void handle();
    void keepAlive();
    void setScheduledReboot(bool wasScheduled);
    void sendTelemetry();
    void sendLog(const String &msg);
};
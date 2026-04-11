#pragma once
#include <Arduino.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#include "LogManager.h"
#include "LightManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"

class BlynkManager {
public:
    void begin(LogManager* logger, LightManager* light, PowerManager* power, TempManager* temp, FanManager* fan, TimeManager* time,const String& fwVer);
    void handle();
    void sendTelemetry();
    void sendLog(const String& msg);
};
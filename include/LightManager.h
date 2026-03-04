#pragma once
#include <Arduino.h>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "TelegramManager.h"
#include <Preferences.h>

class LightManager {
private:
    const int ledPin = 2; // ใช้ LED_BUILTIN จำลองแผง LED 300W ไปก่อน
    const int pwmChannel = 0;
    const int pwmFreq = 5000;
    const int pwmResolution = 8; // 0-255
    LogManager* m_logger = nullptr;
    int brightP1 = 80; // 18.00-20.00
    int brightP2 = 50; // 20.00-00.00
    int brightP3 = 50; // 00.00-04.00
    int brightP4 = 80; // 04.00-06.00
    bool isManualMode = false;
    int manualBrightness = 0;

public:
    void begin(LogManager* sysLogger);
    void targetBrightness(int percent);
    void handle(int currentHour , int currentMinute, TempManager* tm, PowerManager* pm);
    void getBrightness(int &currentPercent);
    void setPeriodBrightness(int period, int percent);
    void setManualMode(bool active, int percent = 0);
};
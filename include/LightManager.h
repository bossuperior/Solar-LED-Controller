#pragma once
#include <Arduino.h>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"

class LightManager {
private:
    const int ledPin = 2; // ใช้ LED_BUILTIN จำลองแผง LED 300W ไปก่อน
    const int pwmChannel = 0;
    const int pwmFreq = 5000;
    const int pwmResolution = 8; // 0-255
    LogManager* m_logger = nullptr;

public:
    void begin(LogManager* sysLogger);
    void targetBrightness(int percent);
    void handle(int currentHour , int currentMinute, TempManager* tm, PowerManager* pm);
    void forceOff() { targetBrightness(0); }
};
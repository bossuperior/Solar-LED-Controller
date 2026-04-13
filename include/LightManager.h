#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Preferences.h>
#include "LogManager.h"
#include "TempManager.h"

#define IR_CODE_ON 0xFFC23D
#define IR_CODE_OFF 0xFFB04F
#define IR_CODE_FULL 0xFF10EF
#define IR_CODE_SEMI 0xFF5AA5
// #define IR_CODE_3H 0xFF22DD
// #define IR_CODE_5H 0xFFA857
// #define IR_CODE_8H 0xFF6897

class LightManager
{
private:
    LogManager *m_logger = nullptr;
    bool isCustomScheduleActive = false;
    bool isManualMode = false;
    bool manualLightState = false;
    bool lastOnState = false;
    bool isTempThrottled = false;
    bool lastThrottleState = false;
    String lightMode = "ปิดไฟ";
    int startHour, startMinute, endHour, endMinute;
    IRsend irsend;
    bool _forceUpdate = false;

public:
    LightManager(uint16_t pin);
    void begin(LogManager *sysLogger);
    void handle(int currentHour, int currentMinute, TempManager *tm);
    void setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable);
    void setManualMode(bool activateManual, bool turnOnLight);
    String isLightMode() { return lightMode; }
    void setScheduleActive(bool enable);
    bool getCustomScheduleActive();
    int getStartHour() const { return startHour; }
    int getStartMin() const { return startMinute; }
    int getEndHour() const { return endHour; }
    int getEndMin() const { return endMinute; }
};
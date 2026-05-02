#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Preferences.h>
#include "LogManager.h"
#include "TempManager.h"
#include "Configs.h"

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
    int startHour = LIGHT_DEFAULT_START_H;
    int startMinute = LIGHT_DEFAULT_START_M;
    int endHour = LIGHT_DEFAULT_END_H;
    int endMinute = LIGHT_DEFAULT_END_M;
    IRsend irsend;
    bool _forceUpdate = false;
    enum PendingIR { IR_NONE, IR_ON_FULL, IR_ON_SEMI, IR_OFF };
    PendingIR _pendingIR = IR_NONE;

public:
    LightManager(uint16_t pin);
    void begin(LogManager *sysLogger);
    void handle(int currentHour, int currentMinute, TempManager *tm);
    void executeIR();
    void setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable);
    void setScheduleParams(int sHour, int sMin, int eHour, int eMin, bool enable);
    void saveScheduleToPrefs();
    void setManualMode(bool activateManual, bool turnOnLight);
    String isLightMode() { return lightMode; }
    void setScheduleActive(bool enable);
    bool getCustomScheduleActive();
    int getStartHour() const { return startHour; }
    int getStartMin() const { return startMinute; }
    int getEndHour() const { return endHour; }
    int getEndMin() const { return endMinute; }
    bool isLightOn() { return lastOnState; }
};
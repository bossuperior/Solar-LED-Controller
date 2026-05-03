#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Preferences.h>
#include <freertos/semphr.h>
#include "LogManager.h"
#include "TempManager.h"
#include "Configs.h"

class LightManager
{
private:
    IRsend irsend;
    LogManager *m_logger = nullptr;
    bool isCustomScheduleActive = false ,isManualMode = false ,manualLightState = false,lastOnState = false,isTempThrottled = false,lastThrottleState = false,_forceUpdate = false;
    int startHour, startMinute, endHour, endMinute;
    enum PendingIR { IR_NONE, IR_ON_FULL, IR_ON_SEMI, IR_OFF };
    PendingIR _pendingIR = IR_NONE;
    bool _pendingNewOnState = false;
    bool _pendingNewThrottleState = false;
    SemaphoreHandle_t _irMutex = nullptr;
    String lightMode = "ปิดไฟ";

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
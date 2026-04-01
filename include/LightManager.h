#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Preferences.h>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"

#define IR_CODE_ON 0xFF02FD
#define IR_CODE_OFF 0xFF9867
#define IR_CODE_FULL 0xFF906F
#define IR_CODE_SEMI 0xFFA857
#define IR_CODE_3H 0xFF08F7
#define IR_CODE_5H 0xFF6897
#define IR_CODE_8H 0xFFB04F

class LightManager
{
private:
    LogManager *m_logger = nullptr;
    IRsend irsend;
    bool isCustomScheduleActive = false;
    bool isManualMode = false;
    bool manualLightState = false;
    bool lastOnState = false;
    String lightMode = "ปิดไฟ";
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;

public:
    LightManager(uint16_t pin);
    void begin(LogManager *sysLogger);
    void handle(int currentHour, int currentMinute, TempManager *tm, PowerManager *pm);
    void setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable);
    void setManualMode(bool activateManual, bool turnOnLight);
    String isLightMode() { return lightMode; }
};
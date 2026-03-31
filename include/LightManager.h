#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"

#define IR_CODE_ON         0xFF02FD
#define IR_CODE_OFF        0xFF9867
#define IR_CODE_FULL       0xFF906F
#define IR_CODE_SEMI       0xFFA857
#define IR_CODE_3H        0xFF08F7
#define IR_CODE_5H        0xFF6897
#define IR_CODE_8H        0xFFB04F

class LightManager {
private:
    LogManager* m_logger = nullptr;
    IRsend irsend;
    bool isCustomScheduleActive = false;
    int startHour = 18;
    int startMinute = 30;
    int endHour = 6;
    int endMinute = 20;
    bool isManualMode = false;
    bool semiLightMode = false;
    bool fullLightMode = false;

public:
    void begin(LogManager* sysLogger);
    void handle(int currentHour , int currentMinute, TempManager* tm, PowerManager* pm);
    void setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable);
};
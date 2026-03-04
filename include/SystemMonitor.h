#pragma once
#include <Arduino.h>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "TelegramManager.h"
#include "LightManager.h"

class SystemMonitor {
private:
    LogManager* m_logger = nullptr;
    unsigned long lastCheck = 0;
    const unsigned long CHECK_INTERVAL = 60000;
    bool errPower = false;
    bool errTemp = false;
    bool errTime = false;
    bool errFan = false;
    bool errDiode = false;
    bool errMosfetShort = false;
    bool errMosfetOpen = false;
    bool errBuckBoost = false;
    bool errBuckHighTemp = false;
    bool errBuckVoltage = false;
    unsigned long fanStartTime = 0; //Variable to track when the fan started

public:
    void begin(LogManager* sysLogger);
    void monitor(PowerManager* pm, TempManager* tm, FanManager* fm, TimeManager* tr, TelegramManager* tg,LightManager* lm);
};
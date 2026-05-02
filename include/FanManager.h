#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "TempManager.h"
#include "LogManager.h"
#include "Configs.h"

class FanManager
{
private:
    int currentSpeed = 0;
    LogManager *m_logger = nullptr;
    float m_tempStart = FAN_DEFAULT_TEMP_START;
    bool m_manualOverride = false;

public:
    void begin(LogManager *sysLogger);
    void handle(TempManager *tm);
    void setFanSpeed(int speed);
    int getFanSpeed();
    bool isFanRunning()
    {
        return currentSpeed > 0;
    }
    void setTempStart(float temp) { m_tempStart = constrain(temp, 30.0f, FAN_DEFAULT_TEMP_MAX - 1.0f); }
    float getTempStart() { return m_tempStart; }
    float getTempMax() { return FAN_DEFAULT_TEMP_MAX; }
    void setManualOverride(bool state) { m_manualOverride = state; }
    bool isManualOverride() { return m_manualOverride; }
    void saveFanSetupToPrefs();
    void loadFanSetupFromPrefs();
};
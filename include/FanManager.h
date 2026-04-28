#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "TempManager.h"
#include "LogManager.h"

class FanManager
{
private:
    const int fanPin = 12;
    const int pwmFreq = 25000; // 25kHz for quieter fan operation
    const int pwmRes = 8;      // 0-255
    const int pwmChannel = 0;
    int currentSpeed = 0;
    LogManager *m_logger = nullptr;
    // Tunable parameters for fan control thresholds and response curve
    float m_tempStart = 38.0; 
    float m_tempMax = 48.0;
    int m_fanMin = 100;
    int m_fanMax = 255;
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
    void setTempStart(float temp) { m_tempStart = constrain(temp, 30.0f, m_tempMax - 1.0f); }
    float getTempStart() { return m_tempStart; }
    float getTempMax() { return m_tempMax; }
    void setManualOverride(bool state) { m_manualOverride = state; }
    bool isManualOverride() { return m_manualOverride; }
    void saveFanSetupToPrefs();
    void loadFanSetupFromPrefs();
    void setCustomFan(float tempStart);
};
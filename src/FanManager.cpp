#include "FanManager.h"
extern Preferences prefs;

void FanManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
    loadFanSetupFromPrefs();
    ledcSetup(pwmChannel, pwmFreq, pwmRes);
    ledcAttachPin(fanPin, pwmChannel);
    setFanSpeed(0);

    if (m_logger != nullptr)
    {
        char initMsg[64];
        snprintf(initMsg, sizeof(initMsg), "Fan Manager initialized on GPIO %d", fanPin);
        m_logger->sysLog("FAN", initMsg);
    }
}

void FanManager::handle(TempManager *tm)
{
    if (tm == nullptr)
        return;

    float buckTemp = tm->getBuckTemp();
    int targetSpeed = currentSpeed;
    bool isSensorError = false;

    if (m_manualOverride) 
    {
        targetSpeed = 255;
    }
    // Safety Mode if sensor reading is invalid (e.g., NaN or negative)
    else if (isnan(buckTemp) || buckTemp < 0.0)
    {
        targetSpeed = 255;
        isSensorError = true;
    }
    // Smart Fan Control Logic with Hysteresis and Linear Mapping
    else
    {
        // Hysteresis: If temperature is below m_tempStart - 2°C, turn off the fan to prevent short cycling
        if (buckTemp < m_tempStart - 2.0)
        {
            targetSpeed = 0;
        }
        else if (buckTemp >= m_tempMax)
        {
            targetSpeed = m_fanMax;
        }
        else if (buckTemp >= m_tempStart)
        {
            float tempRange = m_tempMax - m_tempStart;
            if (tempRange <= 0.0)
            {
                tempRange = 1.0;
            }
            float tempRatio = (buckTemp - m_tempStart) / tempRange;
            targetSpeed = m_fanMin + (int)(tempRatio * (m_fanMax - m_fanMin));
        }
    }
    // Only update fan speed if there's a significant change to reduce wear and noise
    if (abs(targetSpeed - currentSpeed) >= 10 || targetSpeed == 0 || targetSpeed == 255 || isSensorError)
    {

        if (targetSpeed != currentSpeed)
        {
            setFanSpeed(targetSpeed);

            if (m_logger != nullptr)
            {
                if (isSensorError)
                {
                    m_logger->sysLog("FAN", "CRITICAL: Temp sensor error! Forcing Fan ON.");
                }
                else
                {
                    char logMsg[64];
                    if (targetSpeed > 0)
                    {
                        int percent = (targetSpeed * 100) / 255;
                        snprintf(logMsg, sizeof(logMsg), "Temp: %.1fC, Fan Speed: %d%% (%d/255)", buckTemp, percent, targetSpeed);
                    }
                    else
                    {
                        snprintf(logMsg, sizeof(logMsg), "Temp: %.1fC, Fan OFF", buckTemp);
                    }
                    m_logger->sysLog("FAN", logMsg);
                }
            }
        }
    }
}

void FanManager::setFanSpeed(int speed)
{
    currentSpeed = constrain(speed, 0, 255);
    ledcWrite(pwmChannel, currentSpeed);
}

int FanManager::getFanSpeed()
{
    return map(currentSpeed, 0, 255, 0, 100);
}

void FanManager::saveFanSetupToPrefs(){
    prefs.begin("fan_config", false);
    prefs.putFloat("tempStart", m_tempStart);
    prefs.end();
}

void FanManager::loadFanSetupFromPrefs() {
    prefs.begin("fan_config", true);
    m_tempStart = prefs.getFloat("tempStart", 38.0);
    m_manualOverride = false;
    prefs.end();
}
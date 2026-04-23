#include "FanManager.h"

void FanManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;

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

    // Safety Mode if sensor reading is invalid (e.g., NaN or negative)
    if (isnan(buckTemp) || buckTemp < 0.0)
    {
        targetSpeed = 255;
        isSensorError = true;
    }
    // Smart Fan Control Logic with Hysteresis and Linear Mapping
    else
    {
        // Tunable parameters for fan control thresholds and response curve
        const float TEMP_START = 38.0;
        const float TEMP_MAX = 45.0;
        const int FAN_MIN = 100;
        const int FAN_MAX = 255;

        // Hysteresis: If temperature is below TEMP_START - 2°C, turn off the fan to prevent short cycling
        if (buckTemp < TEMP_START - 2.0)
        {
            targetSpeed = 0;
        }
        else if (buckTemp >= TEMP_MAX)
        {
            targetSpeed = FAN_MAX;
        }
        else if (buckTemp >= TEMP_START)
        {
            float tempRange = TEMP_MAX - TEMP_START;
            if (tempRange <= 0.0)
            {
                tempRange = 1.0;
            }
            float tempRatio = (buckTemp - TEMP_START) / tempRange;
            targetSpeed = FAN_MIN + (int)(tempRatio * (FAN_MAX - FAN_MIN));
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
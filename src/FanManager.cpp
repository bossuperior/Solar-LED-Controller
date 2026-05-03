#include "FanManager.h"
extern Preferences prefs;

void FanManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
    loadFanSetupFromPrefs();
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RES);
    ledcAttachPin(FAN_PIN, FAN_PWM_CHANNEL);
    setFanSpeed(0);

    if (m_logger != nullptr)
    {
        char initMsg[64];
        snprintf(initMsg, sizeof(initMsg), "Fan Manager initialized on GPIO %d", FAN_PIN);
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
        targetSpeed = FAN_DEFAULT_MAX_SPEED;
    }
    // Safety Mode if sensor reading is invalid (e.g., NaN or negative)
    else if (isnan(buckTemp) || buckTemp < 0.0)
    {
        targetSpeed = FAN_DEFAULT_MAX_SPEED;
        isSensorError = true;
    }
    // Smart Fan Control Logic with Hysteresis and Linear Mapping
    else
    {
        // Hysteresis: If temperature is below m_tempStart - 2°C, turn off the fan to prevent short cycling
        if (buckTemp < m_tempStart - FAN_HYSTERESIS)
        {
            targetSpeed = 0;
        }
        else if (buckTemp >= FAN_DEFAULT_TEMP_MAX)
        {
            targetSpeed = FAN_DEFAULT_MAX_SPEED;
        }
        else if (buckTemp >= m_tempStart)
        {
            float tempRange = FAN_DEFAULT_TEMP_MAX - m_tempStart;
            if (tempRange <= 0.0)
            {
                tempRange = 1.0;
            }
            float tempRatio = (buckTemp - m_tempStart) / tempRange;
            targetSpeed = FAN_DEFAULT_MIN_SPEED + (int)(tempRatio * (FAN_DEFAULT_MAX_SPEED - FAN_DEFAULT_MIN_SPEED));
        }
    }
    // Only update fan speed if there's a significant change to reduce wear and noise
    if (abs(targetSpeed - currentSpeed) >= FAN_UPDATE_TOLERANCE || targetSpeed == 0 || targetSpeed == FAN_DEFAULT_MAX_SPEED || isSensorError)
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
                        int percent = (targetSpeed * 100) / FAN_DEFAULT_MAX_SPEED;
                        snprintf(logMsg, sizeof(logMsg), "Temp: %.1fC, Fan Speed: %d%% (%d/%d)", buckTemp, percent, targetSpeed, FAN_DEFAULT_MAX_SPEED);
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
    currentSpeed = constrain(speed, 0, FAN_DEFAULT_MAX_SPEED);
    ledcWrite(FAN_PWM_CHANNEL, currentSpeed);
}

int FanManager::getFanSpeed()
{
    return map(currentSpeed, 0, FAN_DEFAULT_MAX_SPEED, 0, 100);
}

void FanManager::saveFanSetupToPrefs(){
    prefs.begin("fan_config", false);
    prefs.putFloat("tempStart", m_tempStart);
    prefs.end();
}

void FanManager::loadFanSetupFromPrefs() {
    prefs.begin("fan_config", true);
    m_tempStart = prefs.getFloat("tempStart", m_tempStart);
    m_manualOverride = false;
    prefs.end();
}
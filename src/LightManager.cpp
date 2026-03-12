#include "LightManager.h"

extern Preferences preferences;

void LightManager::begin(LogManager *sysLoggerPtr)
{
    m_logger = sysLoggerPtr;

    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    targetBrightness(0);
    preferences.begin("light_cfg", true);
    brightP1 = preferences.getInt("p1", 80);
    brightP2 = preferences.getInt("p2", 50);
    brightP3 = preferences.getInt("p3", 50);
    brightP4 = preferences.getInt("p4", 80);
    preferences.end();

    String LightInitMsg = "Light Manager initialized on pin " + String(ledPin) + " with PWM channel " + String(pwmChannel);
    if (m_logger != nullptr)
    {
        m_logger->sysLog("LIGHT", LightInitMsg);
    }
}

void LightManager::targetBrightness(int percent)
{
    // constrain percent to 0-100 and map to duty cycle 0-255
    percent = constrain(percent, 0, 100);
    int dutyCycle = map(percent, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
}

void LightManager::handle(int currentHour, int currentMinute, TempManager *tm, PowerManager *pm)
{
    int currentTotalMinutes = currentHour * 60 + currentMinute;
    int newBrightness = 0;
    if (isManualMode)
    {
        newBrightness = manualBrightness;
    }
    else
    {
        if (currentTotalMinutes >= (18 * 60) && currentTotalMinutes < (20 * 60))
        {
            newBrightness = brightP1;
        }
        else if (currentTotalMinutes >= (20 * 60) && currentTotalMinutes <= (24 * 60))
        {
            newBrightness = brightP2;
        }
        else if (currentTotalMinutes >= 0 && currentTotalMinutes < (4 * 60))
        {
            newBrightness = brightP3;
        }
        else if (currentTotalMinutes >= (4 * 60) && currentTotalMinutes < (6 * 60))
        {
            newBrightness = brightP4;
        }
        else
        {
            newBrightness = 0;
        }
    }

    static bool isTempThrottled = false;
    static bool isBatLow = false;

    if (tm != nullptr)
    {
        float temp = tm->getLedTemp();
        if (temp > 65.0 && temp != 85.0 && temp != -127.0)
        {
            isTempThrottled = true;
        }
        else if (temp < 60.0)
        {
            isTempThrottled = false;
        }
    }
    if (isTempThrottled)
    {
        newBrightness = newBrightness / 2;
    }

    if (pm != nullptr)
    {
        float v = pm->getVoltage();
        if (v <= 3.15)
        {
            isBatLow = true;
        }
        else if (v >= 3.22)
        {
            isBatLow = false;
        }
    }
    if (isBatLow && newBrightness > 0)
    {
        newBrightness = 10;
    }

    static int currentBrightnessState = -1;

    if (newBrightness != currentBrightnessState)
    {
        targetBrightness(newBrightness);
        currentBrightnessState = newBrightness;
        String msg = "Brightness level adjusted to " + String(newBrightness) + "%";
        if (m_logger != nullptr)
        {
            m_logger->sysLog("LIGHT", msg);
        }
    }
}
void LightManager::getBrightness(int &currentPercent)
{
    int dutyCycle = ledcRead(pwmChannel);
    currentPercent = map(dutyCycle, 0, 255, 0, 100);
}

void LightManager::setPeriodBrightness(int period, int percent)
{
    preferences.begin("light_cfg", false);
    if (period == 1)
    {
        brightP1 = percent;
        preferences.putInt("p1", percent);
    }
    else if (period == 2)
    {
        brightP2 = percent;
        preferences.putInt("p2", percent);
    }
    else if (period == 3)
    {
        brightP3 = percent;
        preferences.putInt("p3", percent);
    }
    else if (period == 4)
    {
        brightP4 = percent;
        preferences.putInt("p4", percent);
    }
    preferences.end();

    if (m_logger != nullptr)
    {
        m_logger->sysLog("LIGHT", "Saved Period " + String(period) + " brightness to " + String(percent) + "%");
    }
}
void LightManager::setManualMode(bool active, int percent)
{
    isManualMode = active;
    manualBrightness = constrain(percent, 0, 100);

    if (m_logger != nullptr)
    {
        if (active)
        {
            m_logger->sysLog("LIGHT", "Manual override activated at " + String(manualBrightness) + "%");
        }
        else
        {
            m_logger->sysLog("LIGHT", "Manual override deactivated. Returned to AUTO mode.");
        }
    }
}
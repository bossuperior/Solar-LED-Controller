#include "LightManager.h"

void LightManager::begin(LogManager *sysLoggerPtr)
{
    m_logger = sysLoggerPtr;

    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    targetBrightness(0);

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
    int currentTotalMinutes = (currentHour * 60) + currentMinute;

    // Time thresholds in total minutes
    int timeEvening = (18 * 60) + 0;
    int timeNight = (22 * 60) + 30;
    int timeMorning = (4 * 60) + 15;
    int timeDay = (6 * 60) + 0;
    int newBrightness = 0;

    if (currentTotalMinutes >= timeEvening && currentTotalMinutes < timeNight)
    {
        newBrightness = 80;
    }
    else if (currentTotalMinutes >= timeNight || currentTotalMinutes < timeMorning)
    {
        newBrightness = 50;
    }
    else if (currentTotalMinutes >= timeMorning && currentTotalMinutes < timeDay)
    {
        newBrightness = 80;
    }
    else
    {
        newBrightness = 0;
    }

    if (tm != nullptr && tm->getLEDTemp() > 65.0)
    {
        newBrightness = newBrightness / 2;
    }

    if (pm != nullptr && pm->getVoltage() <= 3.15 && newBrightness > 0)
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
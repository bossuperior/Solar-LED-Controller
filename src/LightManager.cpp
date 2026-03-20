/*
 * Copyright 2026 Komkrit Tungtatiyapat
 *
 * Personal and Educational Use Only.
 * This software is provided for educational and non-commercial purposes. 
 * Any commercial use, modification for commercial purposes, manufacturing, 
 * or distribution for profit is strictly prohibited without prior written 
 * permission from the author.
 * * To request a commercial license, please contact: komkrit.tungtatiyapat@gmail.com
 */

#include "LightManager.h"

extern Preferences preferences;

void LightManager::begin(LogManager *sysLoggerPtr)
{
    m_logger = sysLoggerPtr;

    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    targetBrightness(0);
    preferences.begin("light_cfg", false);
    brightP1 = preferences.getInt("p1", 70);
    brightP2 = preferences.getInt("p2", 80);
    brightP3 = preferences.getInt("p3", 50);
    brightP4 = preferences.getInt("p4", 70);
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
        else if (currentTotalMinutes >= (20 * 60) && currentTotalMinutes < (24 * 60))
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
    bool isSensorError = false;

    if (tm != nullptr)
    {
        float temp = tm->getLedTemp();
        if (temp == -127.0 || temp == 85.0 || temp > 65.0) 
        {
            isTempThrottled = true;
        }
        else if (temp < 60.0 && temp >20)
        {
            isTempThrottled = false;
        }
    }
    if (isTempThrottled == true)
    {
        newBrightness = newBrightness / 2;
    }

    if (pm != nullptr)
    {
        float v = pm->getVoltage();
        if (v < 2.5)
        {
            isSensorError = true;
        }
        else if (v <= 3.15) 
        {
            isBatLow = true;
        }
        else if (v >= 3.22) 
        {
            isBatLow = false;
        }
    }
    if (isSensorError) 
    {
        newBrightness = 0; 
    }
    else if (isBatLow && newBrightness > 0)
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
void LightManager::getBrightness(int &intensityPercent)
{
    int dutyCycle = ledcRead(pwmChannel);
    intensityPercent = map(dutyCycle, 0, 255, 0, 100);
}

void LightManager::setPeriodBrightness(int period, int percent)
{
    percent = constrain(percent, 0, 100); 
    preferences.begin("light_cfg", false);
    int oldVal = 0;
    String keyName = "p" + String(period);
    
    if (period >= 1 && period <= 4) {
        oldVal = preferences.getInt(keyName.c_str(), -1);
        if (oldVal != percent) { 
            if (period == 1) brightP1 = percent;
            else if (period == 2) brightP2 = percent;
            else if (period == 3) brightP3 = percent;
            else if (period == 4) brightP4 = percent;
            preferences.putInt(keyName.c_str(), percent);
            if (m_logger != nullptr) {
                m_logger->sysLog("LIGHT", "Saved Period " + String(period) + " brightness to " + String(percent) + "%");
            }
        }
    }
    
    preferences.end();
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
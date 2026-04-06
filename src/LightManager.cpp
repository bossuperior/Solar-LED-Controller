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

extern Preferences prefs;

LightManager::LightManager(uint16_t pin) : irsend(pin)
{
}

void LightManager::begin(LogManager *sysLoggerPtr)
{
    irsend.begin();

    // --- LOAD PREFERENCES ON BOOT ---
    prefs.begin("light_config", true);
    startHour = prefs.getInt("sHour", 18);
    startMinute = prefs.getInt("sMin", 30);
    endHour = prefs.getInt("eHour", 6);
    endMinute = prefs.getInt("eMin", 20);
    isCustomScheduleActive = prefs.getBool("schActive", false); 
    prefs.end();

    m_logger = sysLoggerPtr;
    if (m_logger != nullptr)
    {
        m_logger->sysLog("LIGHT", "Light Manager initialized");
    }
}

void LightManager::handle(int currentHour, int currentMinute, TempManager *tm, PowerManager *pm)
{
    int currentTotalMinutes = currentHour * 60 + currentMinute;
    int startTotalMinutes = (startHour * 60) + startMinute;
    int endTotalMinutes = (endHour * 60) + endMinute;
    bool shouldBeOn = false;
    if (isManualMode) 
    {
        shouldBeOn = manualLightState;
    }

    else if (isCustomScheduleActive)
    {
        if (startTotalMinutes >= endTotalMinutes) // Handle overnight schedule
        {
            if (currentTotalMinutes >= startTotalMinutes || currentTotalMinutes < endTotalMinutes)
            {
                shouldBeOn = true;
            }
        }
        else
        { // Inday schedule
            if (currentTotalMinutes >= startTotalMinutes && currentTotalMinutes < endTotalMinutes)
            {
                shouldBeOn = true;
            }
        }
    }
    else
    {
        isManualMode = true;
        shouldBeOn = manualLightState;
    }
    bool forceOff = false;

    if (tm != nullptr)
    {
        float temp = tm->getBuckTemp();
        if (temp > 45.0)
        {
            isTempThrottled = true;
        }
        else if (temp < 40.0 && temp > 20)
        {
            isTempThrottled = false;
        }
    }

    if (pm != nullptr)
    {
        float v = pm->getVoltage();
        if (v <= 3.00)
        {
            forceOff = true;
        }
        else if (v <= 3.15)
        {
            isBatLow = true;
        }
        else
        {
            isBatLow = false;
        }
    }
    if (forceOff)
    {
        shouldBeOn = false;
    }
    bool needSemiLight = (isTempThrottled || isBatLow);
    if (shouldBeOn != lastOnState || (forceOff && !wasForcedOff))
    {
        if (shouldBeOn && !forceOff)
        {
            irsend.sendNEC(IR_CODE_ON, 32);
            delay(200);
            if (needSemiLight)
            {
                irsend.sendNEC(IR_CODE_SEMI, 32);
                lightMode = "เปิด(ลดความสว่าง)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Temperature Throttle: Switched to Semi Brightness");
            }
            else
            {
                irsend.sendNEC(IR_CODE_FULL, 32);
                lightMode = "เปิด(สว่างสุด)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Turning ON the Light (Full Brightness)");
            }
        }
        else if (!shouldBeOn)
        {
            irsend.sendNEC(IR_CODE_OFF, 32);
            lightMode = "ปิดไฟ";
            if (m_logger)
            {
                if (forceOff)
                    m_logger->sysLog("LIGHT", "CRITICAL: Battery Low. Forced OFF");
                else
                    m_logger->sysLog("LIGHT", "Turning OFF the Light (Schedule)");
            }
        }
        lastOnState = shouldBeOn;
        lastThrottleState = needSemiLight;
        wasForcedOff = forceOff;
    }
    else if (shouldBeOn && (needSemiLight != lastThrottleState))
    {
        if (needSemiLight)
        {
            irsend.sendNEC(IR_CODE_SEMI, 32);
            lightMode = "เปิด(ลดความสว่าง)";
            if (m_logger)
                m_logger->sysLog("LIGHT", "Safety Triggered: Switched to SEMI Brightness");
        }
        else
        {
            irsend.sendNEC(IR_CODE_FULL, 32);
            lightMode = "เปิด(สว่างสุด)";
            if (m_logger)
                m_logger->sysLog("LIGHT", "Safety Cleared: Switched back to FULL Brightness");
        }
        lastThrottleState = needSemiLight;
    }
}
void LightManager::setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable)
{
    startHour = sHour;
    startMinute = sMin;
    endHour = eHour;
    endMinute = eMin;
    isCustomScheduleActive = enable;
    
    // --- SAVE PREFERENCES WHEN UPDATED ---
    prefs.begin("light_config", false); // 'false' means read/write mode
    prefs.putInt("sHour", startHour);
    prefs.putInt("sMin", startMinute);
    prefs.putInt("eHour", endHour);
    prefs.putInt("eMin", endMinute);
    prefs.putBool("schActive", isCustomScheduleActive);
    prefs.end();

    if (m_logger != nullptr)
    {
        String msg = enable ? "Custom Schedule ON: " + String(sHour) + ":" + String(sMin) + " to " + String(eHour) + ":" + String(eMin) : "Custom Schedule OFF";
        m_logger->sysLog("LIGHT", msg);
    }
}
void LightManager::setManualMode(bool activateManual, bool turnOnLight)
{
    isManualMode = activateManual;
    manualLightState = turnOnLight;
}

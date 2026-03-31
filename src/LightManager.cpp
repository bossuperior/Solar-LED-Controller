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

void LightManager::begin(LogManager *sysLoggerPtr)
{
    irsend.begin();
    m_logger = sysLoggerPtr;

    String LightInitMsg = "Light Manager initialized on pin " + String(ledPin) + " with PWM channel " + String(pwmChannel);
    if (m_logger != nullptr)
    {
        m_logger->sysLog("LIGHT", LightInitMsg);
    }
}

void LightManager::handle(int currentHour, int currentMinute, TempManager *tm, PowerManager *pm)
{
    int currentTotalMinutes = currentHour * 60 + currentMinute;
    int startTotalMinutes = (startHour * 60) + startMinute;
    int endTotalMinutes = (endHour * 60) + endMinute;
    bool shouldBeOn = false;

    if (isCustomScheduleActive)
    {
        if (startTotalMinutes >= endTotalMinutes) //Handle overnight schedule
        {
            if (currentTotalMinutes >= startTotalMinutes || currentTotalMinutes < endTotalMinutes)
            {
                shouldBeOn = true;
            }
        }else{ //Inday schedule
            if (currentTotalMinutes >= startTotalMinutes && currentTotalMinutes < endTotalMinutes) {
                shouldBeOn = true;
            }
        }
        isManualMode = false;
    }
    else{
        isManualMode = true;
    }

    static bool isTempThrottled = false;
    static bool isBatLow = false;
    static bool isSensorError = false;
    bool forceOff = false;

    if (tm != nullptr)
    {
        float temp = tm->getLedTemp();
        if (temp == -127.0 || temp == 85.0 || temp > 65.0)
        {
            isTempThrottled = true;
        }
        else if (temp < 60.0 && temp > 20)
        {
            isTempThrottled = false;
        }
    }

    if (pm != nullptr)
    {
        float v = pm->getVoltage();
        if (v <= 3.00) { 
            forceOff = true; 
        } 
        else if (v <= 3.15) { 
            isBatLow = true; 
        }
        else{
            isBatLow = false; 
        }
    }
    if (forceOff) {
        shouldBeOn = false;

    }
    static bool lastOnState = false;
    static bool lastThrottleState = false;
    static bool wasForcedOff = false;
    bool needSemiLight = (isTempThrottled || isBatLow || isSensorError);
    if (shouldBeOn != lastOnState || (forceOff && !wasForcedOff))
    {
        if (shouldBeOn && !forceOff)
        {
            irsend.sendNEC(IR_CODE_ON, 32);
            delay(200);
            if (needSemiLight)
            {
                irsend.sendNEC(IR_CODE_SEMI, 32);
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Temperature Throttle: Switched to Semi Brightness");
            }
            else
            {
                irsend.sendNEC(IR_CODE_FULL, 32);
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Turning ON the Light (Full Brightness)");
            }
        }
        else if (!shouldBeOn) 
        {
            irsend.sendNEC(IR_CODE_OFF, 32);
            if (m_logger) {
                if (forceOff) m_logger->sysLog("LIGHT", "CRITICAL: Battery Low. Forced OFF");
                else m_logger->sysLog("LIGHT", "Turning OFF the Light (Schedule)");
            }
        }
        lastOnState = shouldBeOn;
        lastThrottleState = needSemiLight;
        wasForcedOff = forceOff;
    }
    else if (shouldBeOn && (needSemiLight != lastThrottleState))
    {
        if (needSemiLight) {
            irsend.sendNEC(IR_CODE_SEMI, 32);
            if (m_logger) m_logger->sysLog("LIGHT", "Safety Triggered: Switched to SEMI Brightness");
        } else {
            irsend.sendNEC(IR_CODE_FULL, 32); 
            if (m_logger) m_logger->sysLog("LIGHT", "Safety Cleared: Switched back to FULL Brightness");
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

    if (m_logger != nullptr)
    {
        String msg = enable ? "Custom Schedule ON: " + String(sHour) + ":" + String(sMin) + " to " + String(eHour) + ":" + String(eMin) : "Custom Schedule OFF";
        m_logger->sysLog("LIGHT", msg);
    }
}

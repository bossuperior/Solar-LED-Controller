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
LightManager::LightManager(uint16_t pin) : irsend(pin) {}

void LightManager::begin(LogManager *sysLoggerPtr)
{
    irsend.begin();

    // --- LOAD PREFERENCES ON BOOT ---
    prefs.begin("light_config", false);
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

void LightManager::handle(int currentHour, int currentMinute, TempManager *tm)
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
        shouldBeOn = false;
    }
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

    bool needSemiLight = isTempThrottled;
    bool stateChanged = (shouldBeOn != lastOnState);
    bool throttleChanged = (shouldBeOn && (needSemiLight != lastThrottleState));

    if (stateChanged || throttleChanged || _forceUpdate)
    {
        _forceUpdate = false;
        if (shouldBeOn)
        {
            irsend.sendNEC(IR_CODE_ON, 32);
            vTaskDelay(pdMS_TO_TICKS(50));
            irsend.sendNEC(IR_CODE_ON, 32);
            vTaskDelay(pdMS_TO_TICKS(150));
            if (needSemiLight)
            {
                irsend.sendNEC(IR_CODE_SEMI, 32);
                vTaskDelay(pdMS_TO_TICKS(50));
                irsend.sendNEC(IR_CODE_SEMI, 32);
                vTaskDelay(pdMS_TO_TICKS(150));
                lightMode = "เปิด(ลดความสว่าง)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Temperature Throttle: Switched to Semi Brightness");
            }
            else
            {
                irsend.sendNEC(IR_CODE_FULL, 32);
                vTaskDelay(pdMS_TO_TICKS(50));
                irsend.sendNEC(IR_CODE_FULL, 32);
                vTaskDelay(pdMS_TO_TICKS(150));
                lightMode = "เปิด(สว่างสุด)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Turning ON the Light (Full Brightness)");
            }
        }
        else if (!shouldBeOn)
        {
            irsend.sendNEC(IR_CODE_OFF, 32);
            vTaskDelay(pdMS_TO_TICKS(50));
            irsend.sendNEC(IR_CODE_OFF, 32);
            vTaskDelay(pdMS_TO_TICKS(150));
            lightMode = "ปิดไฟ";
            if (m_logger)
            {
                m_logger->sysLog("LIGHT", "Turning OFF the Light");
            }
        }
        lastOnState = shouldBeOn;
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
        char logMsg[64];
        if (enable)
        {
            snprintf(logMsg, sizeof(logMsg), "Custom Schedule ON: %02d:%02d to %02d:%02d", sHour, sMin, eHour, eMin);
        }
        else
        {
            snprintf(logMsg, sizeof(logMsg), "Custom Schedule OFF");
        }
        m_logger->sysLog("LIGHT", logMsg);
    }
}

void LightManager::setManualMode(bool activateManual, bool turnOnLight)
{
    isManualMode = activateManual;
    manualLightState = turnOnLight;
    _forceUpdate = true;
}

void LightManager::setScheduleActive(bool enable)
{
    isCustomScheduleActive = enable;
    prefs.begin("light_config", false);
    prefs.putBool("schActive", isCustomScheduleActive);
    prefs.end();

    if (m_logger)
    {
        m_logger->sysLog("LIGHT", enable ? "Auto Schedule: ENABLED" : "Auto Schedule: DISABLED");
    }
}

bool LightManager::getCustomScheduleActive() { return isCustomScheduleActive; }
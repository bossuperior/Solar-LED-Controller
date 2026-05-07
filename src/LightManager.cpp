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
    _irMutex = xSemaphoreCreateMutex();
    irsend.begin();

    // --- LOAD PREFERENCES ON BOOT ---
    prefs.begin("light_config", true);
    startHour = prefs.getInt("sHour", LIGHT_DEFAULT_START_H);
    startMinute = prefs.getInt("sMin", LIGHT_DEFAULT_START_M);
    endHour = prefs.getInt("eHour", LIGHT_DEFAULT_END_H);
    endMinute = prefs.getInt("eMin", LIGHT_DEFAULT_END_M);
    isCustomScheduleActive = prefs.getBool("schActive", false);
    prefs.end();

    _forceUpdate = true;
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
        if (startTotalMinutes > endTotalMinutes) // Handle overnight schedule
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
        if (temp > TEMP_THROTTLE_ON)
        {
            isTempThrottled = true;
        }
        else if (!isnan(temp) && temp < TEMP_THROTTLE_OFF)
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
        PendingIR newIR = IR_NONE;
        if (shouldBeOn)
        {
            if (needSemiLight)
            {
                newIR = IR_ON_SEMI;
                lightMode = "เปิด(ลดความสว่าง)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Temperature Throttle: Switched to Semi Brightness");
            }
            else
            {
                newIR = IR_ON_FULL;
                lightMode = "เปิด(สว่างสุด)";
                if (m_logger)
                    m_logger->sysLog("LIGHT", "Turning ON the Light (Full Brightness)");
            }
        }
        else
        {
            newIR = IR_OFF;
            lightMode = "ปิดไฟ";
            if (m_logger)
                m_logger->sysLog("LIGHT", "Turning OFF the Light");
        }
        if (_irMutex && xSemaphoreTake(_irMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            _pendingIR = newIR;
            _pendingNewOnState = shouldBeOn;
            _pendingNewThrottleState = needSemiLight;
            xSemaphoreGive(_irMutex);
        }
    }
}

void LightManager::executeIR()
{
    if (_pendingIR == IR_NONE)
        return;

    if (!_irMutex || xSemaphoreTake(_irMutex, pdMS_TO_TICKS(5)) != pdTRUE)
        return;

    PendingIR toExecute = _pendingIR;
    bool newOnState = _pendingNewOnState;
    bool newThrottleState = _pendingNewThrottleState;
    _pendingIR = IR_NONE;
    xSemaphoreGive(_irMutex);

    if (toExecute == IR_NONE)
        return;

    switch (toExecute)
    {
    case IR_ON_FULL:
        irsend.sendNEC(IR_CODE_ON, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(50));
        irsend.sendNEC(IR_CODE_ON, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        irsend.sendNEC(IR_CODE_FULL, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(50));
        irsend.sendNEC(IR_CODE_FULL, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        break;
    case IR_ON_SEMI:
        irsend.sendNEC(IR_CODE_ON, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(50));
        irsend.sendNEC(IR_CODE_ON, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        irsend.sendNEC(IR_CODE_SEMI, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(50));
        irsend.sendNEC(IR_CODE_SEMI, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        break;
    case IR_OFF:
        irsend.sendNEC(IR_CODE_OFF, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(50));
        irsend.sendNEC(IR_CODE_OFF, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        irsend.sendNEC(IR_CODE_OFF, IR_BITS);
        vTaskDelay(pdMS_TO_TICKS(150));
        break;
    default:
        break;
    }
    lastOnState = newOnState;
    lastThrottleState = newThrottleState;
}

void LightManager::setScheduleParams(int sHour, int sMin, int eHour, int eMin, bool enable)
{
    if (startHour == sHour && startMinute == sMin &&
        endHour == eHour && endMinute == eMin &&
        isCustomScheduleActive == enable)
        return;
    startHour = sHour;
    startMinute = sMin;
    endHour = eHour;
    endMinute = eMin;
    isCustomScheduleActive = enable;
    _forceUpdate = true;
}

void LightManager::saveScheduleToPrefs()
{
    prefs.begin("light_config", false);
    prefs.putInt("sHour", startHour);
    prefs.putInt("sMin", startMinute);
    prefs.putInt("eHour", endHour);
    prefs.putInt("eMin", endMinute);
    prefs.putBool("schActive", isCustomScheduleActive);
    prefs.end();

    if (m_logger != nullptr)
    {
        char logMsg[64];
        if (isCustomScheduleActive)
            snprintf(logMsg, sizeof(logMsg), "Custom Schedule ON: %02d:%02d to %02d:%02d", startHour, startMinute, endHour, endMinute);
        else
            snprintf(logMsg, sizeof(logMsg), "Custom Schedule OFF");
        m_logger->sysLog("LIGHT", logMsg);
    }
}

void LightManager::setCustomSchedule(int sHour, int sMin, int eHour, int eMin, bool enable)
{
    setScheduleParams(sHour, sMin, eHour, eMin, enable);
    saveScheduleToPrefs();
}

void LightManager::setManualMode(bool activateManual, bool turnOnLight)
{
    if (isManualMode == activateManual && manualLightState == turnOnLight)
        return;
    isManualMode = activateManual;
    manualLightState = turnOnLight;
    _forceUpdate = true;
}

void LightManager::setScheduleActive(bool enable)
{
    if (isCustomScheduleActive == enable)
        return;
    isCustomScheduleActive = enable;
    _forceUpdate = true;

    if (m_logger)
    {
        m_logger->sysLog("LIGHT", enable ? "Auto Schedule: ENABLED" : "Auto Schedule: DISABLED");
    }
}

bool LightManager::getCustomScheduleActive() { return isCustomScheduleActive; }
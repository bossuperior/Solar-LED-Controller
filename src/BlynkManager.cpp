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

#include "secret.h"
#define BLYNK_TEMPLATE_ID SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN SECRET_BLYNK_AUTH_TOKEN
#include <BlynkSimpleEsp32.h>
#include "BlynkManager.h"

// --- GLOBAL POINTERS FOR BLYNK CALLBACKS ---
BlynkManager *b_manager = nullptr;
LightManager *b_light = nullptr;
PowerManager *b_power = nullptr;
TempManager *b_temp = nullptr;
FanManager *b_fan = nullptr;
LogManager *b_logger = nullptr;
TimeManager *b_time = nullptr;
OTAManager *b_ota = nullptr;
SemaphoreHandle_t *b_mutex = nullptr;
String b_fwVer = "";

static bool pendingOtaUpdate = false;
static bool pendingOtaRollback = false;
static bool b_scheduledReboot = false;

// V0: switchonoff
BLYNK_WRITE(V0)
{
    int state = param.asInt();
    if (b_light && b_power && b_mutex)
    {
        if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (state == 1)
                b_light->setManualMode(true, true);
            else
                b_light->setManualMode(false, false);
            xSemaphoreGive(*b_mutex);
        }
        Blynk.virtualWrite(V8, state == 1 ? "💡 เปิดไฟแล้ว\n" : "🌑 ปิดไฟ (กลับสู่โหมดตั้งเวลา)\n");
    }
}

// V1: timelight (schedule time input)
BLYNK_WRITE(V1)
{
    TimeInputParam t(param);

    if (t.hasStartTime() && t.hasStopTime())
    {
        int sH = t.getStartHour();
        int sM = t.getStartMinute();
        int eH = t.getStopHour();
        int eM = t.getStopMinute();

        if (b_light && b_mutex)
        {
            if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                b_light->setScheduleParams(sH, sM, eH, eM, b_light->getCustomScheduleActive());
                b_light->saveScheduleToPrefs();
                xSemaphoreGive(*b_mutex);
            }
        }

        char msgBuffer[128];
        snprintf(msgBuffer, sizeof(msgBuffer), "⏰ อัปเดตเวลาออโต้: %d:%02d น. ถึง %d:%02d น.\n", sH, sM, eH, eM);
        Blynk.virtualWrite(V8, msgBuffer);
    }
    else
    {
        Blynk.virtualWrite(V8, "⚠️ กรุณาตั้งค่าเวลาให้ครบทั้งเริ่มและสิ้นสุด!\n");
    }
}

BLYNK_CONNECTED()
{
    Blynk.syncVirtual(V1, V9, V10);
    if (b_light)
    {
        Blynk.virtualWrite(V0, b_light->isLightOn() ? 1 : 0);
        Blynk.virtualWrite(V3, b_light->getCustomScheduleActive() ? 1 : 0);
    }

    if (b_logger)
    {
        if (b_scheduledReboot)
        {
            Blynk.virtualWrite(V8, "🔄 รีบู๊ตระบบรายสัปดาห์เรียบร้อย\n");
            b_scheduledReboot = false;
        }
        else
        {
            Blynk.virtualWrite(V8, "🔄 ระบบออนไลน์ โหลดข้อมูลสำเร็จ\n");
        }
    }
}

BLYNK_WRITE(V3)
{
    int state = param.asInt();
    if (b_light && b_mutex)
    {
        bool isEnabled = (state == 1);
        if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            b_light->setManualMode(false, false);
            b_light->setScheduleActive(isEnabled);
            b_light->saveScheduleToPrefs();
            xSemaphoreGive(*b_mutex);
        }
        Blynk.virtualWrite(V8, isEnabled ? "⏰ เปิดระบบตั้งเวลาอัตโนมัติ\n" : "🛑 ปิดระบบตั้งเวลาอัตโนมัติ\n");
    }
}

BLYNK_WRITE(V9)
{
    int switchState = param.asInt();
    if (b_fan && b_mutex)
    {
        if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            b_fan->setManualOverride(switchState == 1);
            xSemaphoreGive(*b_mutex);
        }
        Blynk.virtualWrite(V8, switchState == 1 ? "🌀 เปิดพัดลมแล้ว\n" : "🌀 ปิดพัดลม (กลับสู่โหมดออโต้)\n");
    }
}

BLYNK_WRITE(V10)
{
    float newStart = param[0].asFloat();
    newStart = constrain(newStart, 30.0f, 45.0f);
    if (b_fan && b_mutex)
    {
        if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            b_fan->setTempStart(newStart);
            b_fan->saveFanSetupToPrefs();
            xSemaphoreGive(*b_mutex);
        }
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "🌀 อัปเดตอุณหภูมิที่พัดลมเริ่มทำงาน: %.1f °C\n", newStart);
        Blynk.virtualWrite(V8, logMsg);
    }
}

// V11: Check OTA from GitHub
BLYNK_WRITE(V11)
{
    if (param.isEmpty())
        return;
    int state = param.asInt();
    if (state == 1)
    {
        pendingOtaUpdate = true;
        Blynk.virtualWrite(V11, 0); // Reset button in UI
    }
}
// V12: Trigger OTA rollback
BLYNK_WRITE(V12)
{
    if (param.isEmpty())
        return;
    int state = param.asInt();
    if (state == 1)
    {
        pendingOtaRollback = true;
        Blynk.virtualWrite(V12, 0); // Reset button in UI
    }
}

void BlynkManager::keepAlive()
{
    if (Blynk.connected())
        Blynk.run();
}

void BlynkManager::setScheduledReboot(bool wasScheduled)
{
    b_scheduledReboot = wasScheduled;
}

void BlynkManager::begin(LogManager *logger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, TimeManager *time, SemaphoreHandle_t *mutex, OTAManager *ota, const String &fwVer)
{
    b_logger = logger;
    b_light = light;
    b_power = power;
    b_temp = temp;
    b_fan = fan;
    b_time = time;
    b_mutex = mutex;
    b_ota = ota;
    b_fwVer = fwVer;

    b_manager = this;
    Blynk.config(BLYNK_AUTH_TOKEN);
    if (b_logger)
        b_logger->sysLog("BLYNK", "Blynk IoT Initialized");
}

void BlynkManager::handle()
{
    if (!Blynk.connected())
    {
        Blynk.connect(3000);
    }
    Blynk.run();

    if (pendingOtaUpdate)
    {
        pendingOtaUpdate = false;
        if (b_ota != nullptr && b_manager != nullptr && b_power != nullptr)
        {
            b_ota->checkUpdate(b_fwVer, b_logger, b_power, b_manager, true);
        }
    }

    if (pendingOtaRollback)
    {
        pendingOtaRollback = false;
        if (b_ota != nullptr && b_manager != nullptr)
        {
            b_ota->triggerRollback(b_manager, b_logger);
        }
    }
}

void BlynkManager::sendTelemetry()
{
    if (!b_power || !b_temp || !b_fan || !b_light)
        return;
    if (!Blynk.connected())
        return;

    float v = b_power->getVoltage();
    float tBuck = b_temp->getBuckTemp();
    float tChip = b_temp->getChipTemp();
    int fanSpeed = b_fan->getFanSpeed();
    int freeRam_kB = ESP.getFreeHeap() / 1024;

    // System uptime
    uint64_t uptimeSecs = esp_timer_get_time() / 1000000;
    int days = uptimeSecs / 86400;
    int hours = (uptimeSecs % 86400) / 3600;
    int mins = (uptimeSecs % 3600) / 60;
    String ssid = WiFi.SSID();
    ssid.replace("|", "-");
    char sysInfo[128];
    snprintf(sysInfo, sizeof(sysInfo), "WiFi: %s | Uptime: %dd %dh %dm | FW: %s",
             ssid.c_str(), days, hours, mins, b_fwVer.c_str());

    static float last_v = -100.0;
    static float last_tBuck = -100.0;
    static float last_tChip = -100.0;
    static String last_chip_color = "";
    static int last_fanSpeed = -1;
    static int last_mins = -1;
    static int last_schActive = -1;
    static int last_ram = -1;
    static int last_pushed_v0 = -1;
    static uint8_t same_v_count = 0;
    static String last_v_color = "";
    static String last_t_color = "";
    int current_physical_state = b_light->isLightOn() ? 1 : 0;

    if (current_physical_state != last_pushed_v0)
    {
        Blynk.virtualWrite(V0, current_physical_state);
        last_pushed_v0 = current_physical_state;
    }

    bool currentSchActive = b_light->getCustomScheduleActive();
    if (currentSchActive != last_schActive)
    {
        Blynk.virtualWrite(V3, currentSchActive ? 1 : 0);
        last_schActive = currentSchActive;
    }

    String current_v_color;
    if (v < BATT_CRITICAL_LOW_V)
    {
        current_v_color = COLOR_CRITICAL;
    }
    else if (v >= BATT_CRITICAL_LOW_V && v < UI_BATT_WARN_V)
    {
        current_v_color = COLOR_WARNING;
    }
    else
    {
        current_v_color = COLOR_NORMAL;
    }
    if (current_v_color != last_v_color)
    {
        Blynk.setProperty(V2, "color", current_v_color);
        last_v_color = current_v_color;
    }
    // Dashboard Update: Only send updates if values have changed significantly to reduce network traffic
    bool volt_changed = fabsf(v - last_v) >= BLYNK_DELTA_VOLT;
    same_v_count = volt_changed ? 0 : same_v_count + 1;
    if (volt_changed || same_v_count >= BLYNK_SAME_VOLT_COUNT)
    {
        Blynk.virtualWrite(V2, v);
        last_v = v;
        same_v_count = 0;
    }

    float startTemp = b_fan->getTempStart();
    float maxTemp = b_fan->getTempMax();
    String current_t_color;

    if (tBuck < startTemp)
    {
        current_t_color = COLOR_WHITE;
    }
    else if (tBuck >= startTemp && tBuck < maxTemp)
    {
        current_t_color = COLOR_WARNING;
    }
    else
    {
        current_t_color = COLOR_CRITICAL;
    }
    if (current_t_color != last_t_color)
    {
        Blynk.setProperty(V4, "color", current_t_color);
        last_t_color = current_t_color;
    }
    if (fabsf(tBuck - last_tBuck) >= BLYNK_DELTA_TEMP)
    {
        Blynk.virtualWrite(V4, tBuck);
        last_tBuck = tBuck;
    }

    if (fanSpeed != last_fanSpeed)
    {
        Blynk.virtualWrite(V5, fanSpeed);
        last_fanSpeed = fanSpeed;
    }

    String current_chip_color;
    if (tChip >= ALERT_CHIP_TEMP_CRITICAL)
        current_chip_color = COLOR_CRITICAL;
    else if (tChip >= ALERT_CHIP_TEMP_WARNING)
        current_chip_color = COLOR_WARNING;
    else
        current_chip_color = COLOR_WHITE;
    if (current_chip_color != last_chip_color)
    {
        Blynk.setProperty(V13, "color", current_chip_color);
        last_chip_color = current_chip_color;
    }
    if (fabsf(tChip - last_tChip) >= BLYNK_DELTA_CHIP_TEMP)
    {
        Blynk.virtualWrite(V13, tChip);
        last_tChip = tChip;
    }

    if (mins != last_mins)
    {
        Blynk.virtualWrite(V7, sysInfo);
        last_mins = mins;
    }
    if (abs(freeRam_kB - last_ram) >= BLYNK_DELTA_RAM_KB)
    {
        Blynk.virtualWrite(V6, freeRam_kB);
        last_ram = freeRam_kB;
    }
}

void BlynkManager::sendLog(const String &msg)
{
    char logBuffer[512];
    snprintf(logBuffer, sizeof(logBuffer), "%s\n", msg.c_str());
    Blynk.virtualWrite(V8, logBuffer);
}
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

BlynkManager *b_manager = nullptr;
LightManager *b_light = nullptr;
PowerManager *b_power = nullptr;
FanManager *b_fan = nullptr;
LogManager *b_logger = nullptr;
SemaphoreHandle_t *b_mutex = nullptr;

BLYNK_WRITE(InternalPinOTA)
{
    if (!b_manager)
        return;
    String otaUrl = param.asString();
    if (!otaUrl.startsWith("https://") && !otaUrl.startsWith("http://"))
    {
        Blynk.virtualWrite(V8, "❌ ไม่สามารถอัพเดตได้: ลิงก์ไม่ถูกต้อง\n");
        return;
    }
    b_manager->setPendingOTA(otaUrl);
    Blynk.virtualWrite(V8, "🚀 เริ่มการอัพเดตเฟิร์มแวร์ใหม่...\n⚠️ อย่าปิดไฟหรือรีเซ็ตบอร์ด!\n");
}

void BlynkManager::checkOTA()
{
    if (pendingOTA)
    {
        pendingOTA = false; // Reset the flag to prevent multiple triggers
        Blynk.virtualWrite(V8, "📡 กำลังดาวน์โหลดเฟิร์มแวร์ใหม่...\n");
        WiFiClientSecure client;
        client.setInsecure();
        httpUpdate.rebootOnUpdate(false);
        httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

        esp_task_wdt_delete(NULL); // suspend WDT for this task during download
        client.setTimeout(60);

        t_httpUpdate_return ret = httpUpdate.update(client, otaDownloadUrl);
        esp_task_wdt_add(NULL); // re-register WDT on failure path

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            char errMsg[160];
            snprintf(errMsg, sizeof(errMsg),
                     "❌ Error (%d): %s\n",
                     httpUpdate.getLastError(),
                     httpUpdate.getLastErrorString().c_str());
            Blynk.virtualWrite(V8, errMsg);
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Blynk.virtualWrite(V8, "⚠️ ไม่มีอัปเดตบนเซิร์ฟเวอร์\n");
            break;
        case HTTP_UPDATE_OK:
            Blynk.virtualWrite(V8, "✅ ดาวน์โหลดและเช็คไฟล์สำเร็จ!\n🔄 กำลังรีบูตเครื่อง...");
            delay(3000);
            ESP.restart();
            break;
        }
    }
}

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
            Blynk.virtualWrite(V8, state == 1 ? "💡 เปิดไฟแล้ว\n" : "🌑 ปิดไฟ (กลับสู่โหมดตั้งเวลา)\n");
        }
        else
        {
            Blynk.virtualWrite(V8, "⚠️ ระบบไม่ว่างกรุณาลองเปิดปิดไฟใหม่!\n");
            Blynk.virtualWrite(V0, !state); // Revert the switch state in the app since we couldn't process it
        }
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
                // Preserve current active state — V3 controls enable/disable exclusively
                b_light->setScheduleParams(sH, sM, eH, eM, b_light->getCustomScheduleActive());
                xSemaphoreGive(*b_mutex);
                b_light->saveScheduleToPrefs(); // NVS write outside mutex
                char msgBuffer[128];
                snprintf(msgBuffer, sizeof(msgBuffer), "⏰ อัปเดตเวลาออโต้: %d:%02d น. ถึง %d:%02d น.\n", sH, sM, eH, eM);
                Blynk.virtualWrite(V8, msgBuffer);
            }
        }
    }
    else
    {
        Blynk.virtualWrite(V8, "⚠️ กรุณาตั้งค่าเวลาให้ครบทั้งเริ่มและสิ้นสุด!\n");
    }
}

BLYNK_CONNECTED()
{
    Blynk.syncVirtual(V0, V1, V3, V9, V10);

    if (b_logger)
    {
        Blynk.virtualWrite(V8, "🔄 ระบบออนไลน์ โหลดข้อมูลจากคลาวด์สำเร็จ\n");
    }
}

BLYNK_WRITE(V3)
{
    int state = param.asInt();
    if (b_light && b_mutex)
    {
        bool isEnabled = (state == 1);
        if (xSemaphoreTake(*b_mutex, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            b_light->setManualMode(false, false);
            b_light->setScheduleActive(isEnabled);
            xSemaphoreGive(*b_mutex);
            b_light->saveScheduleToPrefs(); // NVS write outside mutex
            Blynk.virtualWrite(V8, isEnabled ? "⏰ เปิดระบบตั้งเวลาอัตโนมัติ\n" : "🛑 ปิดระบบตั้งเวลาอัตโนมัติ\n");
        }
        else
        {
            Blynk.virtualWrite(V8, "⚠️ ระบบไม่ว่างกรุณาลองกดปุ่มตั้งเวลาใหม่!\n");
            Blynk.virtualWrite(V3, !state); // Revert the switch state in the app since we couldn't process it
        }
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
            Blynk.virtualWrite(V8, switchState == 1 ? "🌀 เปิดพัดลมแล้ว\n" : "🌀 ปิดพัดลม (กลับสู่โหมดออโต้)\n");
        }
        else
        {
            Blynk.virtualWrite(V8, "⚠️ ระบบไม่ว่างกรุณาลองเปิดปิดพัดลมใหม่!\n");
            Blynk.virtualWrite(V9, !switchState);
        }
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
            xSemaphoreGive(*b_mutex);
            b_fan->saveFanSetupToPrefs(); // NVS write outside mutex
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "🌀 อัปเดตอุณหภูมิที่พัดลมเริ่มทำงาน: %.1f °C\n", newStart);
            Blynk.virtualWrite(V8, logMsg);
        }
    }
}

void BlynkManager::begin(LogManager *logger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, TimeManager *time, SemaphoreHandle_t *mutex, const String &fwVer)
{
    b_logger = logger;
    b_light = light;
    b_power = power;
    b_temp = temp;
    b_fan = fan;
    b_time = time;
    b_mutex = mutex;
    b_fwVer = fwVer;

    b_manager = this;
    Blynk.config(BLYNK_AUTH_TOKEN);
    if (b_logger)
        b_logger->sysLog("BLYNK", "Blynk IoT Initialized");
}

void BlynkManager::handle()
{
    Blynk.run();
}

void BlynkManager::sendTelemetry()
{
    if (!b_power || !b_temp || !b_fan || !b_light || !b_time)
        return;

    float v = b_power->getVoltage();
    float tBuck = b_temp->getBuckTemp();
    int fanSpeed = b_fan->getFanSpeed();
    int freeRam_kB = ESP.getFreeHeap() / 1024;

    // System uptime
    uint64_t uptimeSecs = esp_timer_get_time() / 1000000;
    int days = uptimeSecs / 86400;
    int hours = (uptimeSecs % 86400) / 3600;
    int mins = (uptimeSecs % 3600) / 60;
    char sysInfo[128];
    snprintf(sysInfo, sizeof(sysInfo), "WiFi: %s | Uptime: %dd %dh %dm | FW: %s",
             WiFi.SSID().c_str(), days, hours, mins, b_fwVer.c_str());

    static float last_v = -100.0;
    static float last_tBuck = -100.0;
    static int last_fanSpeed = -1;
    static int last_mins = -1;
    static int last_schActive = -1;
    static int last_ram = -1;
    static int last_pushed_v0 = -1;
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
    if (v < 3.10)
    {
        current_v_color = "#FF4444";
    }
    else if (v >= 3.10 && v < 3.20)
    {
        current_v_color = "#FFB300";
    }
    else
    {
        current_v_color = "#00E676";
    }
    if (current_v_color != last_v_color)
    {
        Blynk.setProperty(V2, "color", current_v_color);
        last_v_color = current_v_color;
    }
    // Dashboard Update: Only send updates if values have changed significantly to reduce network traffic
    if (abs(v - last_v) >= 0.02)
    {
        Blynk.virtualWrite(V2, v);
        last_v = v;
    }

    float startTemp = b_fan->getTempStart();
    float maxTemp = b_fan->getTempMax();
    String current_t_color;

    if (tBuck < startTemp)
    {
        current_t_color = "#FFFFFF";
    }
    else if (tBuck >= startTemp && tBuck < maxTemp)
    {
        current_t_color = "#FFB300";
    }
    else
    {
        current_t_color = "#FF4444";
    }
    if (current_t_color != last_t_color)
    {
        Blynk.setProperty(V4, "color", current_t_color);
        last_t_color = current_t_color;
    }
    if (abs(tBuck - last_tBuck) >= 0.2)
    {
        Blynk.virtualWrite(V4, tBuck);
        last_tBuck = tBuck;
    }

    if (fanSpeed != last_fanSpeed)
    {
        Blynk.virtualWrite(V5, fanSpeed);
        last_fanSpeed = fanSpeed;
    }

    if (mins != last_mins)
    {
        Blynk.virtualWrite(V7, sysInfo);
        last_mins = mins;
    }
    if (abs(freeRam_kB - last_ram) >= 1)
    {
        Blynk.virtualWrite(V6, freeRam_kB);
        last_ram = freeRam_kB;
    }
}

void BlynkManager::sendLog(const String &msg)
{
    Blynk.virtualWrite(V8, msg + "\n");
}
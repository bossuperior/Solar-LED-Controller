#include "secret.h"
#define BLYNK_TEMPLATE_ID SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN SECRET_BLYNK_AUTH_TOKEN
#include <BlynkSimpleEsp32.h>
#include "BlynkManager.h"

// --- GLOBAL POINTERS FOR BLYNK CALLBACKS ---
LightManager *b_light = nullptr;
PowerManager *b_power = nullptr;
TempManager *b_temp = nullptr;
FanManager *b_fan = nullptr;
LogManager *b_logger = nullptr;
TimeManager *b_time = nullptr;
String b_fwVer = "";

BLYNK_WRITE(InternalPinOTA)
{
    String otaUrl = param.asString();

    Blynk.virtualWrite(V8, "========================\n");
    Blynk.virtualWrite(V8, "🚀 กำลังดาวน์โหลดเฟิร์มแวร์ใหม่...\n");
    Blynk.virtualWrite(V8, "⚠️ กรุณาอย่าปิดไฟหรือรีเซ็ตบอร์ด!\n");

    WiFiClientSecure client;
    client.setInsecure();
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    t_httpUpdate_return ret = httpUpdate.update(client, otaUrl);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Blynk.virtualWrite(V8, "❌ อัปเดตล้มเหลว: " + httpUpdate.getLastErrorString() + "\n");
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Blynk.virtualWrite(V8, "⚠️ ไม่มีอัปเดต\n");
        break;
    case HTTP_UPDATE_OK:
        Blynk.virtualWrite(V8, "✅ อัปเดตสำเร็จ! กำลังรีบูตระบบใน 3 วินาที...\n");
        delay(3000);
        ESP.restart();
        break;
    }
}

// V0: switchonoff
BLYNK_WRITE(V0)
{
    int state = param.asInt();
    if (b_light && b_power)
    {
        if (state == 1 && b_power->isPowerSafe())
        {
            b_light->setManualMode(true, true);
            Blynk.virtualWrite(V8, "💡 เปิดไฟแล้ว\n");
        }
        else if (state == 1 && !b_power->isPowerSafe())
        {
            Blynk.virtualWrite(V0, 0);
            Blynk.virtualWrite(V8, "⚠️ แบตเตอรี่ต่ำ บล็อกการเปิดไฟ!\n");
        }
        else
        {
            b_light->setManualMode(false, false);
            Blynk.virtualWrite(V8, "🌑 ปิดไฟ กลับเข้าสู่โหมดตั้งเวลา\n");
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

        if (b_light)
        {
            b_light->setCustomSchedule(sH, sM, eH, eM, true);
        }

        String startStr = String(sH) + ":" + (sM < 10 ? "0" : "") + String(sM);
        String endStr = String(eH) + ":" + (eM < 10 ? "0" : "") + String(eM);
        String msg = "⏰ อัปเดตเวลาออโต้: " + startStr + " น. ถึง " + endStr + " น.\n";

        Blynk.virtualWrite(V8, msg);
    }
    else
    {
        Blynk.virtualWrite(V8, "⚠️ กรุณาตั้งค่าเวลาให้ครบทั้งเริ่มและสิ้นสุด!\n");
    }
}

BLYNK_CONNECTED()
{
    Blynk.syncVirtual(V0, V1, V3);

    if (b_logger)
    {
        Blynk.virtualWrite(V8, "🔄 ระบบออนไลน์ ซิงก์ข้อมูลล่าสุดจากคลาวด์สำเร็จ\n");
    }
}

BLYNK_WRITE(V3)
{
    int state = param.asInt();
    if (b_light)
    {
        b_light->setScheduleActive(state == 1);
        Blynk.virtualWrite(V8, state == 1 ? "⏰ เปิดระบบตั้งเวลาอัตโนมัติ\n" : "🛑 ปิดระบบตั้งเวลาอัตโนมัติ\n");
    }
}

void BlynkManager::begin(LogManager *logger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, TimeManager *time, String fwVer)
{
    b_logger = logger;
    b_light = light;
    b_power = power;
    b_temp = temp;
    b_fan = fan;
    b_time = time;
    b_fwVer = fwVer;

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

    // System uptime
    uint64_t uptimeSecs = esp_timer_get_time() / 1000000;
    int days = uptimeSecs / 86400;
    int hours = (uptimeSecs % 86400) / 3600;
    int mins = (uptimeSecs % 3600) / 60;
    String sysInfo = "WiFi: " + WiFi.SSID() + " | Uptime: " + String(days) + "d " + String(hours) + "h " + String(mins) + "m | FW: " + b_fwVer;

    static float last_v = -100.0;
    static float last_tBuck = -100.0;
    static int last_fanSpeed = -1;
    static int last_mins = -1;
    static String last_lightStatus = "";
    static int last_schActive = -1;

    String currentLightStatus = b_light->isLightMode();

    if (currentLightStatus != last_lightStatus)
    {
        if (currentLightStatus.indexOf("เปิด") >= 0)
        {
            Blynk.virtualWrite(V0, 1);
        }
        else
        {
            Blynk.virtualWrite(V0, 0);
        }
        last_lightStatus = currentLightStatus;
    }

    bool currentSchActive = b_light->getCustomScheduleActive();
    if (currentSchActive != last_schActive)
    {
        Blynk.virtualWrite(V3, currentSchActive ? 1 : 0);
        last_schActive = currentSchActive;
    }

    // Dashboard Update: Only send updates if values have changed significantly to reduce network traffic
    if (abs(v - last_v) >= 0.03)
    {
        Blynk.virtualWrite(V2, v);
        last_v = v;
    }

    if (abs(tBuck - last_tBuck) >= 0.4)
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
}

void BlynkManager::sendLog(String msg)
{
    Blynk.virtualWrite(V8, msg + "\n");
}
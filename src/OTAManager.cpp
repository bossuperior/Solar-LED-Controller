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

#include "OTAManager.h"

extern Preferences prefs;

void OTAManager::checkUpdate(String currentVersion, LogManager *sysLogger, PowerManager *pm, BlynkManager *bk, bool force)
{
    isUpdating = true;
    m_logger = sysLogger;
    m_power = pm;
    m_blynk = bk;
    float voltage = pm->getVoltage();
    int rssi = WiFi.RSSI();
    if (force == false)
    {
        if (pm->isInaAvailable() && voltage < 0.1)
        {
            if (m_logger)
                m_logger->sysLog("OTA", "Abort: Power sensor unreliable or 0.00V detected!");
            if (m_blynk)
                m_blynk->sendLog("❌ อัปเดตล้มเหลว: แรงดันไม่พอ(0.00V) กรุณาเช็คสายไฟหรือแบตเตอรี่");
            isUpdating = false;
            return;
        }
    }
    if (voltage > 0.1 && voltage < 3.15)
    {
        if (m_logger)
            m_logger->sysLog("OTA", "Abort: Battery too low (" + String(voltage) + "V)");
        if (m_blynk && force)
            m_blynk->sendLog("❌ ยกเลิกการอัปเดต: แบตเตอรี่ต่ำเกินไป (" + String(voltage) + "V)");
        isUpdating = false;
        return;
    }
    if (rssi < -85)
    {
        if (m_logger)
            m_logger->sysLog("OTA", "Abort: WiFi signal too weak (" + String(rssi) + "dBm)");
        if (m_blynk && force)
            m_blynk->sendLog("❌ ยกเลิกการอัปเดต: สัญญาณ WiFi อ่อนเกินไป (" + String(rssi) + "dBm)");
        isUpdating = false;
        return;
    }
    esp_task_wdt_reset();
    if (WiFi.status() != WL_CONNECTED)
    {
        if (m_logger)
            m_logger->sysLog("OTA", "Skip check: WiFi not connected.");
        isUpdating = false;
        return;
    }
    if (m_logger)
        m_logger->sysLog("OTA", "Checking GitHub for new release...");
    if (m_blynk)
        m_blynk->sendLog("🔍 กำลังตรวจสอบอัปเดตจากเซิร์ฟเวอร์...");
    WiFi.setSleep(false);
    esp_task_wdt_reset();
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10000);
    client.setHandshakeTimeout(10000);
    HTTPClient http;
    http.begin(client, SECRET_OTA_UPDATE_API);
    http.addHeader("User-Agent", "ESP32-OTA");

    String latestTag = "";
    esp_task_wdt_reset();
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        int tagIndex = payload.indexOf("\"tag_name\":\"");
        if (tagIndex != -1)
        {
            latestTag = payload.substring(tagIndex + 12);
            latestTag = latestTag.substring(0, latestTag.indexOf("\""));
            if (latestTag.startsWith("v") || latestTag.startsWith("V"))
                latestTag = latestTag.substring(1);

            if (m_logger != nullptr)
            {
                m_logger->sysLog("OTA", "Local Version: " + currentVersion + " | Server Version: " + latestTag);
            }

            if (latestTag == currentVersion)
            {
                if (force == false)
                {
                    if (m_logger != nullptr)
                    {
                        m_logger->sysLog("OTA", "Firmware is up to date. Skipping...");
                        if (m_blynk)
                            m_blynk->sendLog("✅ เฟิร์มแวร์เป็นเวอร์ชันล่าสุดแล้ว (" + currentVersion + ")");
                    }
                    http.end();
                    WiFi.setSleep(true);
                    isUpdating = false;
                    return;
                }
                else
                {
                    if (m_logger)
                        m_logger->sysLog("OTA", "Force update! Re-flashing the same version.");
                    if (m_blynk)
                        m_blynk->sendLog("⚠️ บังคับอัปเดตทับเวอร์ชันเดิม (" + latestTag + ")");
                }
            }
        }
        else
        {
            if (m_logger)
                m_logger->sysLog("OTA", "Invalid JSON payload from GitHub API.");
            if (m_blynk && force)
                m_blynk->sendLog("❌ ข้อผิดพลาด: รูปแบบข้อมูลจากเซิร์ฟเวอร์ไม่ถูกต้อง");
            http.end();
            WiFi.setSleep(true);
            isUpdating = false;
            return;
        }
    }
    else
    {
        if (m_logger != nullptr)
        {
            m_logger->sysLog("OTA", "Failed to fetch API. Error: " + String(httpCode));
        }
        if (m_blynk != nullptr && force)
            m_blynk->sendLog("❌ ข้อผิดพลาด: เชื่อมต่อเซิร์ฟเวอร์ล้มเหลว (Code: " + String(httpCode) + ")");
        http.end();
        WiFi.setSleep(true);
        isUpdating = false;
        return;
    }
    http.end();
    if (m_logger)
        m_logger->sysLog("OTA", "New version found! Starting download...");
    if (m_blynk)
        m_blynk->sendLog("📡 พบเวอร์ชัน " + latestTag + " อย่าปิดเบรคเกอร์หรือรีเซ็ตบอร์ด!");
    httpUpdate.onProgress([](size_t current, size_t total)
                          { esp_task_wdt_reset(); });
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false);
    WiFi.setSleep(false);
    esp_task_wdt_reset();
    t_httpUpdate_return ret = httpUpdate.update(client, SECRET_OTA_UPDATE_URL);
    WiFi.setSleep(true);
    if (ret != HTTP_UPDATE_OK)
    {
        isUpdating = false;
    }

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        if (m_logger)
            m_logger->sysLog("OTA", "Update failed. Error (" + String(httpUpdate.getLastError()) + "): " + httpUpdate.getLastErrorString());
        if (m_blynk && force)
            m_blynk->sendLog("❌ การติดตั้งเฟิร์มแวร์ล้มเหลว: " + httpUpdate.getLastErrorString());
        break;
    case HTTP_UPDATE_NO_UPDATES:
        if (m_logger)
            m_logger->sysLog("OTA", "No new updates available.");
        if (m_blynk)
            m_blynk->sendLog("⚠️ ไม่มีอัปเดตบนเซิร์ฟเวอร์");
        break;
    case HTTP_UPDATE_OK:
        prefs.begin("app_info", false);
        prefs.putString("fw_ver", latestTag);
        prefs.end();
        if (m_logger)
            m_logger->sysLog("OTA", "Update successful! Rebooting...");
        if (m_blynk)
        {
            m_blynk->sendLog("✅ อัปเดตเรียบร้อย! กำลังรีสตาร์ทเป็นเวอร์ชัน " + latestTag);
            delay(1500);
        }
        ESP.restart();
        break;
    }
}
void OTAManager::triggerRollback(BlynkManager* bk)
{
    if (bk != nullptr) {
        m_blynk = bk;
    }
    if (Update.canRollBack())
    {
        if (m_logger) m_logger->sysLog("SYSTEM", "Rolling back to previous version...");
        if (m_blynk) m_blynk->sendLog("⚠️ กำลังย้อนกลับไปเวอร์ชันก่อนหน้า...");
        
        if (Update.rollBack())
        {
            if (m_logger) m_logger->sysLog("SYSTEM", "Rollback successful! Rebooting...");
            if (m_blynk) {
                m_blynk->sendLog("✅ ย้อนกลับเวอร์ชันสำเร็จ! กำลังรีสตาร์ท...");
                delay(1500);
            }
            delay(2000);
            ESP.restart();
        }
        else
        {
            if (m_logger) m_logger->sysLog("ERROR", "Rollback failed!");
            if (m_blynk) m_blynk->sendLog("❌ การย้อนกลับเวอร์ชันล้มเหลว!");
        }
    }
    else
    {
        if (m_logger) m_logger->sysLog("ERROR", "No previous version available to rollback.");
        if (m_blynk) m_blynk->sendLog("❌ ไม่มีเฟิร์มแวร์เวอร์ชันก่อนหน้าให้ย้อนกลับ!");
    }
}
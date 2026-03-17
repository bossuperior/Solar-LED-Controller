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

extern Preferences preferences;

void OTAManager::begin()
{
    preferences.begin("ota_safety", true);
    _isWaitingValidation = preferences.getBool("pending_validate", false);
    preferences.end();
    if (_isWaitingValidation && m_logger)
    {
        m_logger->sysLog("SYSTEM", "New firmware detected. Safety timer started (5 mins).");
    }
}
void OTAManager::checkUpdate(String currentVersion, LogManager *sysLogger, PowerManager *pm, bool force)
{
    isUpdating = true;
    m_logger = sysLogger;
    m_power = pm;
    float voltage = pm->getVoltage();
    int rssi = WiFi.RSSI();
    if (force == false)
    {
        if (pm->isInaAvailable() && voltage < 0.1)
        {
            if (m_logger)
                m_logger->sysLog("OTA", "Abort: Power sensor unreliable or 0.00V detected!");
            isUpdating = false;
            return;
        }
    }
    if (voltage > 0.1 && voltage < 3.5)
    {
        if (m_logger)
            m_logger->sysLog("OTA", "Abort: Battery too low (" + String(voltage) + "V)");
        isUpdating = false;
        return;
    }
    if (rssi < -85)
    {
        if (m_logger)
            m_logger->sysLog("OTA", "Abort: WiFi signal too weak (" + String(rssi) + "dBm)");
        isUpdating = false;
        return;
    }
    esp_task_wdt_reset();
    if (WiFi.status() != WL_CONNECTED)
    {
        if (m_logger != nullptr)
        {
            isUpdating = false;
            m_logger->sysLog("OTA", "Skip check: WiFi not connected.");
        }
        return;
    }
    if (m_logger != nullptr)
    {
        m_logger->sysLog("OTA", "Checking GitHub for new release...");
    }
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);
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
                    }
                    http.end();
                    isUpdating = false;
                    return;
                }
                else 
                {
                    if (m_logger != nullptr)
                    {
                        m_logger->sysLog("OTA", "Force update! Re-flashing the same version.");
                    }
                }
            }
        }
        else
        {
            if (m_logger != nullptr)
            {
                m_logger->sysLog("OTA", "Invalid JSON payload from GitHub API.");
            }
            http.end();
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
        http.end();
        isUpdating = false;
        return;
    }
    http.end();
    if (m_logger != nullptr)
    {
        m_logger->sysLog("OTA", "New version found! Starting download...");
    }
    httpUpdate.onProgress([](size_t current, size_t total)
                          { esp_task_wdt_reset(); });
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false);
    esp_task_wdt_reset();
    t_httpUpdate_return ret = httpUpdate.update(client, SECRET_OTA_UPDATE_URL);
    if (ret != HTTP_UPDATE_OK)
    {
        isUpdating = false;
    }

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        if (m_logger != nullptr)
        {
            m_logger->sysLog("OTA", "Update failed. Error (" + String(httpUpdate.getLastError()) + "): " + httpUpdate.getLastErrorString());
        }
        break;
    case HTTP_UPDATE_NO_UPDATES:
        if (m_logger != nullptr)
        {
            m_logger->sysLog("OTA", "No new updates available.");
        }
        break;
    case HTTP_UPDATE_OK:
        preferences.begin("ota_safety", false);
        preferences.putBool("pending_validate", true);
        preferences.end();
        preferences.begin("app_info", false);
        preferences.putString("fw_ver", latestTag);
        preferences.end();
        if (m_logger != nullptr)
        {
            m_logger->sysLog("OTA", "Update successful! Rebooting...");
        }
        delay(1000);
        ESP.restart();
        break;
    }
}
void OTAManager::triggerRollback()
{
    if (Update.canRollBack())
    {
        m_logger->sysLog("SYSTEM", "Rolling back to previous version...");
        if (Update.rollBack())
        {
            m_logger->sysLog("SYSTEM", "Rollback successful! Rebooting...");
            delay(2000);
            ESP.restart();
        }
        else
        {
            m_logger->sysLog("ERROR", "Rollback failed!");
        }
    }
    else
    {
        m_logger->sysLog("ERROR", "No previous version available to rollback.");
    }
}
void OTAManager::handleSafetyTimer()
{
    if (_isWaitingValidation)
    {
        if (millis() > 300000)
        {
            triggerRollback();
        }
    }
}

void OTAManager::validateUpdate()
{
    if (_isWaitingValidation)
    {
        preferences.begin("ota_safety", false);
        preferences.putBool("pending_validate", false);
        preferences.end();
        _isWaitingValidation = false;
        if (m_logger)
            m_logger->sysLog("SYSTEM", "New firmware validated successfully!");
    }
}
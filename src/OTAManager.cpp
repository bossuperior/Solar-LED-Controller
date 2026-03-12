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

void OTAManager::checkUpdate(String currentVersion, LogManager *sysLogger)
{
    isUpdating = true;
    m_logger = sysLogger;
    esp_task_wdt_reset();
    if (WiFi.status() != WL_CONNECTED) {
        if (m_logger != nullptr) {
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
                if (m_logger != nullptr)
                {
                    m_logger->sysLog("OTA", "Firmware is up to date. Skipping...");
                }
                http.end();
                isUpdating = false;
                return;
            }
        }else {
            if(m_logger != nullptr) {
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
    httpUpdate.onProgress([](size_t current, size_t total) {
        esp_task_wdt_reset();
    });
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false);
    esp_task_wdt_reset();
    t_httpUpdate_return ret = httpUpdate.update(client, SECRET_OTA_UPDATE_URL);
    if (ret != HTTP_UPDATE_OK) {
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
        preferences.begin("app_info", false);
        preferences.putString("fw_ver", latestTag);
        preferences.end();
        if (m_logger != nullptr)
        {
            m_logger->sysLog("OTA", "Update successful! Rebooting...");
        }
        delay(500);
        ESP.restart();
        break;
    }
}
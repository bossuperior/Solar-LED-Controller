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

#include "NetworkManager.h"

void NetworkManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);
    delay(100);
    wifiMulti.addAP(SECRET_SSID_HOME, SECRET_PASS_HOME);
    wifiMulti.addAP(SECRET_SSID_BOSS, SECRET_PASS_BOSS);

    _startAttemptTime = millis();
    _apModeStarted = false;
}

void NetworkManager::handle()
{
    //AP-Mode
    static unsigned long lastScanTime = 0;
    uint8_t currentStatus = WiFi.status();
    if (_apModeStarted)
    {
        if (millis() - lastScanTime > NET_CHECK_INTERVAL)
        {
            if (m_logger)
                m_logger->sysLog("NETWORK", "Checking if Home WiFi is back...");
            currentStatus = wifiMulti.run();
            lastScanTime = millis();

            if (currentStatus == WL_CONNECTED)
            {
                WiFi.softAPdisconnect(true);
                delay(100);
                WiFi.mode(WIFI_STA);
                WiFi.setSleep(true);
                _apModeStarted = false;
                _startAttemptTime = millis();
                if (m_logger)
                    m_logger->sysLog("NETWORK", "Home WiFi is back! Returning to STA mode.");
                return;
            }
        }
        return;
    }
    // Normal STA mode handling
    currentStatus = wifiMulti.run();
    if (currentStatus != WL_CONNECTED)
    {
        if (millis() - _startAttemptTime > WIFI_MAX_ATTEMPT_TIME)
        {
            if (m_logger != nullptr)
            {
                m_logger->sysLog("NETWORK", "WiFi Not Found. Starting AP Fallback Mode.");
            }
            WiFi.disconnect();
            delay(100);
            WiFi.mode(WIFI_AP_STA);
            WiFi.setSleep(false);
            delay(100);
            bool apStatus = WiFi.softAP(AP_SSID, SECRET_AP_PASS);
            if (apStatus && m_logger != nullptr)
            {
                char apMsg[64];
                snprintf(apMsg, sizeof(apMsg), "AP Started: %s, IP: %s", AP_SSID, WiFi.softAPIP().toString().c_str());
                m_logger->sysLog("NETWORK", apMsg);
            }
            else if (m_logger != nullptr)
            {
                m_logger->sysLog("NETWORK", "ERROR: Failed to start AP Mode!");
            }
            _apModeStarted = apStatus;
            if (!apStatus)
                _startAttemptTime = millis();
            lastScanTime = millis();
        }
    }

    if (currentStatus != lastStatus)
    {
        if (currentStatus == WL_CONNECTED)
        {
            if (m_logger != nullptr)
            {
                char connMsg[128];
                snprintf(connMsg, sizeof(connMsg), "WiFi Connected to SSID: %s, IP: %s",
                         WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                m_logger->sysLog("NETWORK", connMsg);
            }
        }
        else if (lastStatus == WL_CONNECTED)
        {
            _startAttemptTime = millis();
            if (m_logger != nullptr && !_apModeStarted)
            {
                m_logger->sysLog("NETWORK", "WiFi Connection Lost / Seeking...");
            }
        }
        lastStatus = (wl_status_t)currentStatus;
    }
}

bool NetworkManager::isInternetAvailable()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        _hasInternet = false;
        _firstCheck = true;
        return false;
    }

    if (_firstCheck || (millis() - lastNetCheck > INTERNET_CHECK_INTERVAL))
    {
        lastNetCheck = millis();
        _firstCheck = false;
        esp_task_wdt_reset();
        WiFiClient client;
        HTTPClient http;
        http.begin(client,"http://clients3.google.com/generate_204");
        http.setTimeout(2000);
        int httpCode = http.GET();
        esp_task_wdt_reset();
        bool currentInternetStatus = false;
        if (httpCode > 0)
        {
            currentInternetStatus = (httpCode == 204);
            if (currentInternetStatus != _hasInternet)
            {
                if (currentInternetStatus && m_logger != nullptr)
                {
                    m_logger->sysLog("NETWORK", "Internet is fully accessible.");
                }
                else if (!currentInternetStatus && m_logger != nullptr)
                {
                    m_logger->sysLog("NETWORK", "Connected to WiFi but no Internet access.");
                }
            }
        }
        else
        {
            currentInternetStatus = false;
            if (currentInternetStatus != _hasInternet && m_logger != nullptr)
            {
                char errMsg[64];
                snprintf(errMsg, sizeof(errMsg), "GET request failed, error: %s", http.errorToString(httpCode).c_str());
                m_logger->sysLog("NETWORK", errMsg);
            }
        }
        _hasInternet = currentInternetStatus;
        http.end();
    }
    return _hasInternet;
}
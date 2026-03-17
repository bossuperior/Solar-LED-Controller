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
    WiFi.mode(WIFI_STA);
    delay(100);
    wifiMulti.addAP(SECRET_SSID_HOME, SECRET_PASS_HOME);
    wifiMulti.addAP(SECRET_SSID_BOSS, SECRET_PASS_BOSS);

    _startAttemptTime = millis();
    _apModeStarted = false;
}

void NetworkManager::handle()
{
    static unsigned long lastScanTime = 0;
    uint8_t currentStatus = WiFi.status();
    if (_apModeStarted) 
    {
        if (millis() - lastScanTime > 10000) 
        {
            currentStatus = wifiMulti.run();
            lastScanTime = millis();
        }
    } 
    else 
    {
        currentStatus = wifiMulti.run();
    }
    if (currentStatus != WL_CONNECTED && !_apModeStarted)
    {
        if (millis() - _startAttemptTime > maxAttemptTime)
        {
            if (m_logger != nullptr)
            {
                m_logger->sysLog("NETWORK", "WiFi Not Found. Starting AP Fallback Mode.");
            }
            WiFi.disconnect(); 
            delay(100);
            WiFi.mode(WIFI_AP_STA);
            delay(100);
            bool apStatus = WiFi.softAP("T_SOLAR_LED_AP",SECRET_AP_PASS);
            if (apStatus && m_logger != nullptr) {
                m_logger->sysLog("NETWORK", "AP Started: T_SOLAR_LED_AP, IP: " + WiFi.softAPIP().toString());
            } else if (m_logger != nullptr) {
                m_logger->sysLog("NETWORK", "ERROR: Failed to start AP Mode!");
            }
            _apModeStarted = true;
            lastScanTime = millis();
        }
    }
    else if (currentStatus == WL_CONNECTED && _apModeStarted)
    {
        WiFi.softAPdisconnect(true); 
        delay(100);
        WiFi.mode(WIFI_STA);
        _apModeStarted = false;
        if (m_logger != nullptr)
        {
            m_logger->sysLog("NETWORK", "Internet can accesible! Closed AP and returned to STA mode.");
        }
    }

    if (currentStatus != lastStatus)
    {
        if (currentStatus == WL_CONNECTED)
        {
            if (m_logger != nullptr)
            {
                m_logger->sysLog("NETWORK", "WiFi Connected to SSID: " + WiFi.SSID() + ", IP: " + WiFi.localIP().toString());
            }
        }
        else
        {
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

    if (_firstCheck || (millis() - lastNetCheck > netInterval))
    {
        lastNetCheck = millis();
        _firstCheck = false;
        esp_task_wdt_reset();
        HTTPClient http;
        http.begin("http://clients3.google.com/generate_204");
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
                m_logger->sysLog("NETWORK", "GET request failed, error: " + http.errorToString(httpCode));
            }
        }
        _hasInternet = currentInternetStatus;
        http.end();
    }
    return _hasInternet;
}
#include "NetworkManager.h"
#include "secret.h"

void NetworkManager::begin(LogManager* sysLoggerPtr)
{
    m_logger = sysLoggerPtr;
    WiFi.mode(WIFI_STA);
    delay(100);
    wifiMulti.addAP(SECRET_SSID_HOME, SECRET_PASS_HOME);
    wifiMulti.addAP(SECRET_SSID_BOSS, SECRET_PASS_BOSS);
}

void NetworkManager::handle()
{
    uint8_t currentStatus = wifiMulti.run();
    if (currentStatus != lastStatus)
    {
        if (currentStatus == WL_CONNECTED)
        {
            if (m_logger != nullptr) {
                m_logger->sysLog("NETWORK", "WiFi Connected to SSID: " + WiFi.SSID()+", IP: " + WiFi.localIP().toString());
            }
        }
        else
        {
            if (m_logger != nullptr) { 
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
        HTTPClient http;
        http.begin("http://clients3.google.com/generate_204");
        http.setTimeout(2000);
        int httpCode = http.GET();
        bool currentInternetStatus = false;
        if (httpCode > 0)
        {
            currentInternetStatus = (httpCode == 204);
            if (currentInternetStatus != _hasInternet) {
                if (currentInternetStatus && m_logger != nullptr) {
                    m_logger->sysLog("NETWORK", "Internet is fully accessible.");
                }
                else if (!currentInternetStatus && m_logger != nullptr) {
                    m_logger->sysLog("NETWORK", "Connected to WiFi but no Internet access.");
                }
            }
        }
        else
        {
            currentInternetStatus = false;
            if (currentInternetStatus != _hasInternet && m_logger != nullptr) {
                m_logger->sysLog("NETWORK", "GET request failed, error: " + http.errorToString(httpCode));
            }
        }
        _hasInternet = currentInternetStatus;
        http.end();
    }
    return _hasInternet;
}
#include "NetworkManager.h"
#include "secret.h"

void NetworkManager::begin()
{
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
            Serial.println("\nWiFi Connected!");
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println("\nWiFi Connection Lost / Seeking...");
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
        if (httpCode > 0)
        {
            _hasInternet = (httpCode == 204);
            if (_hasInternet)
            {
                Serial.println("[Net] Internet is fully accessible.");
            }
            else
            {
                Serial.printf("[Net] Server responded with unexpected code: %d\n", httpCode);
            }
        }
        else
        {
            _hasInternet = false;
            Serial.printf("[Net] GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return _hasInternet;
}
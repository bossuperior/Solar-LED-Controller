#include "NetworkManager.h"
#include "secret.h"
#include <HTTPClient.h>

void NetworkManager::begin()
{
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

bool NetworkManager::isInternetAvailable() {
    if (WiFi.status() != WL_CONNECTED) {
        _hasInternet = false;
        return false;
    }

    if (millis() - lastNetCheck > netInterval) {
        lastNetCheck = millis();
        HTTPClient http;
        http.begin("http://clients3.google.com/generate_204");
        http.setTimeout(2000); 
        int httpCode = http.GET();
        http.end();
        
        _hasInternet = (httpCode == 204);
        
        if (!_hasInternet) {
            Serial.println("[Net] Router connected but No Internet! Waiting for recovery...");
        } else {
            Serial.println("[Net] Internet is fully accessible.");
        }
    }
    return _hasInternet;
}
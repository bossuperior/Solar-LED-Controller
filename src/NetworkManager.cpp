#include "NetworkManager.h"
#include "secret.h"

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
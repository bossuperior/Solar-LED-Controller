#include "OTAManager.h"
#include "secret.h"

void OTAManager::checkUpdate(String currentVersion)
{
    Serial.println("[OTA] Checking GitHub for new release...");
    
    WiFiClientSecure client;
    client.setInsecure(); 
    client.setTimeout(12000);
    HTTPClient http;
    http.begin(client, SECRET_OTA_UPDATE_API); 
    http.addHeader("User-Agent", "ESP32-OTA");
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        int tagIndex = payload.indexOf("\"tag_name\":\"");
        if (tagIndex != -1) {
            String latestTag = payload.substring(tagIndex + 12);
            latestTag = latestTag.substring(0, latestTag.indexOf("\""));

            Serial.printf("[OTA] Local Version: %s | Server Version: %s\n", currentVersion.c_str(), latestTag.c_str());

            if (latestTag == currentVersion) {
                Serial.println("[OTA] Firmware is up to date. Skipping...");
                http.end();
                return;
            }
        }
    } else {
        Serial.printf("[OTA] Failed to fetch API. Error: %d\n", httpCode);
    }
    http.end();

    Serial.println("[OTA] New version found! Starting download...");
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    t_httpUpdate_return ret = httpUpdate.update(client, SECRET_OTA_UPDATE_URL);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update failed. Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] No new updates available.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Update successful! Rebooting...");
            break;
    }
}
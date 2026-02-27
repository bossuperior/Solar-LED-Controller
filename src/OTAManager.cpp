#include "OTAManager.h"
#include "secret.h"

void OTAManager::checkUpdate(String currentVersion)
{
    Serial.println("[OTA] Checking GitHub for new release...");
    
    WiFiClientSecure client;
    client.setInsecure(); 
    client.setTimeout(12000);

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
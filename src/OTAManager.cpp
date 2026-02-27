#include "OTAManager.h"
#include "secret.h"

void OTAManager::checkUpdate(const char* currentVersion) {
    if (isUpdating) return;
    Serial.printf("[OTA] Current Version: %s\n", currentVersion);
    Serial.println("[OTA] Checking GitHub for new release...");

    isUpdating = true;
    WiFiClientSecure client;
    client.setInsecure();
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    _updateUrl = SECRET_OTA_UPDATE_URL;
    t_httpUpdate_return ret = httpUpdate.update(client, _updateUrl);

    if (ret == HTTP_UPDATE_FAILED || ret == HTTP_UPDATE_NO_UPDATES) {
        isUpdating = false;
        Serial.printf("[OTA] Process finished. Status: %d\n", ret);
    }
}
#include "GsheetManager.h"
void GsheetManager::begin(LogManager *sysLogger, TimeManager *timeManager)
{
    m_sysLogger = sysLogger;
    m_timeManager = timeManager;
    m_client.setInsecure();
    if (m_sysLogger != nullptr)
    {
        m_sysLogger->sysLog("GSHEET", "Google Sheets manager initialized");
    }
}
void GsheetManager::sendData(float voltage, float tempBuck, int fanSpeed, const String &lightPct)
{
    String currentTime = m_timeManager->getCurrentTime();
    currentTime.trim();

    if (currentTime.startsWith("1970"))
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Skip: Time not synced yet.");
        return;
    }
    if (voltage < 0.1 || tempBuck <= -100.0)
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Skip: Incomplete sensor data.");
        return;
    }
    char jsonPayload[256];
    snprintf(jsonPayload, sizeof(jsonPayload),
             "{\"time\":\"%s\",\"voltage\":%.2f,\"tempBuck\":%.1f,\"fanSpeed\":%d,\"lightPct\":\"%s\"}",
             currentTime.c_str(), voltage, tempBuck, fanSpeed, lightPct.c_str());

    HTTPClient http;
    String url = SECRET_GOOGLE_SHEET_URL;

    if (url.isEmpty())
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Error: Script URL is empty.");
        return;
    }

    http.begin(m_client, url);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    esp_task_wdt_reset();
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode == 200 || httpResponseCode == 302)
    {
        if (m_sysLogger != nullptr)
        {
            char logMsg[64];
            snprintf(logMsg, sizeof(logMsg), "Success! Data saved. (Code: %d)", httpResponseCode);
            m_sysLogger->sysLog("GSHEET", logMsg);
        }
    }
    else
    {
        if (m_sysLogger != nullptr)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Error sending data. Code: %d", httpResponseCode);
            m_sysLogger->sysLog("GSHEET", errorMsg);
        }
    }
    http.end();
    esp_task_wdt_reset();
}
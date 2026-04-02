#include "GsheetManager.h"
void GsheetManager::begin(LogManager* sysLogger, TimeManager* timeManager)
{
    m_sysLogger = sysLogger;
    m_timeManager = timeManager;
    m_client.setInsecure();
    if (m_sysLogger != nullptr)
    {
        m_sysLogger->sysLog("GSHEET", "Google Sheets manager initialized");
    }
}
void GsheetManager::sendData(float voltage, float tempLed, float tempBuck, int fanSpeed, String lightPct)
{
    String currentTime = m_timeManager->getCurrentTime();

    if (currentTime.startsWith("1970")) 
    {
        if (m_sysLogger != nullptr) m_sysLogger->sysLog("GSHEET", "Skip: Time not synced yet.");
        return;
    }
    if (voltage < 0.1 || tempLed <= -100.0 || tempBuck <= -100.0) 
    {
        if (m_sysLogger != nullptr) m_sysLogger->sysLog("GSHEET", "Skip: Incomplete sensor data.");
        return;
    }
    String jsonPayload = "{";
    jsonPayload += "\"time\":\"" + currentTime + "\",";
    jsonPayload += "\"voltage\":" + String(voltage, 2) + ",";
    jsonPayload += "\"tempLed\":" + String(tempLed, 1) + ",";
    jsonPayload += "\"tempBuck\":" + String(tempBuck, 1) + ",";
    jsonPayload += "\"fanSpeed\":" + String(fanSpeed) + ",";
    jsonPayload += "\"lightPct\":\"" + lightPct + "\"";
    jsonPayload += "}";

    HTTPClient http;
    String url = SECRET_GOOGLE_SHEET_URL;

    if (url.isEmpty())
    {
        if (m_sysLogger != nullptr) m_sysLogger->sysLog("GSHEET", "Error: Script URL is empty.");
        return;
    }

    http.begin(m_client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 
    http.addHeader("Content-Type", "application/json");
    esp_task_wdt_reset();
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0)
    {
        if (m_sysLogger != nullptr)
        {
            m_sysLogger->sysLog("GSHEET", "Data sent to Google Sheets. Response code: " + String(httpResponseCode));
        }
    }
    else
    {
        if (m_sysLogger != nullptr)
        {
            m_sysLogger->sysLog("GSHEET", "Error sending data. HTTP error: " + http.errorToString(httpResponseCode));
        }
    }
    http.end();
    esp_task_wdt_reset();
}
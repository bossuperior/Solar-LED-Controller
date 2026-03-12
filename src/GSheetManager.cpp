#include "GsheetManager.h"
void GsheetManager::begin(LogManager* sysLogger, TimeManager* timeManager)
{
    m_sysLogger = sysLogger;
    m_timeManager = timeManager;
    if (m_sysLogger != nullptr)
    {
        m_sysLogger->sysLog("GSHEET", "Google Sheets manager initialized");
    }
}
void GsheetManager::sendData(float voltage, float tempLed, float tempBuck, int fanSpeed, int lightPct)
{
    String postData = "Time=" + m_timeManager->getCurrentTime() + "&" +
                      "voltage=" + String(voltage, 2) + "&" +
                      "tempLed=" + String(tempLed, 1) + "&" +
                      "tempBuck=" + String(tempBuck, 1) + "&" +
                      "fanSpeed=" + String(fanSpeed) + "&" +
                      "lightPct=" + String(lightPct);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String url = SECRET_GOOGLE_SHEET_URL;

    if (url.isEmpty())
    {
        if (m_sysLogger != nullptr) m_sysLogger->sysLog("GSHEET", "Error: Script URL is empty.");
        return;
    }

    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    esp_task_wdt_reset();
    int httpResponseCode = http.POST(postData);
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
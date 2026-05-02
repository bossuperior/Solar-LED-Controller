#include "GsheetManager.h"

static String jsonEscapeString(const String &s)
{
    String out;
    out.reserve(s.length() + 8);
    for (unsigned int i = 0; i < s.length(); i++)
    {
        char c = s[i];
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

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
    if (m_timeManager == nullptr) return;
    String currentTime = m_timeManager->getCurrentTime();
    currentTime.trim();

    if (currentTime.startsWith("1970"))
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Skip: Time not synced yet.");
        return;
    }
    if (voltage < 0.1 || isnan(tempBuck) || tempBuck <= -100.0)
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Skip: Incomplete sensor data.");
        return;
    }
    String escapedLight = jsonEscapeString(lightPct);
    char jsonPayload[GSHEET_JSON_BUFFER_SIZE];
    snprintf(jsonPayload, sizeof(jsonPayload),
             "{\"time\":\"%s\",\"voltage\":%.2f,\"tempBuck\":%.1f,\"fanSpeed\":%d,\"lightPct\":\"%s\"}",
             currentTime.c_str(), voltage, tempBuck, fanSpeed, escapedLight.c_str());

    HTTPClient http;
    String url = SECRET_GOOGLE_SHEET_URL;

    if (url.isEmpty())
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("GSHEET", "Error: Script URL is empty.");
        return;
    }

    http.begin(m_client, url);
    http.setTimeout(GSHEET_HTTP_TIMEOUT);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    esp_task_wdt_reset();
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode == 200)
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
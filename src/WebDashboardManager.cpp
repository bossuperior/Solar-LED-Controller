/*
 * Copyright 2026 Komkrit Tungtatiyapat
 *
 * Personal and Educational Use Only.
 * This software is provided for educational and non-commercial purposes.
 * Any commercial use, modification for commercial purposes, manufacturing,
 * or distribution for profit is strictly prohibited without prior written
 * permission from the author.
 * * To request a commercial license, please contact: komkrit.tungtatiyapat@gmail.com
 */

#include "WebDashboardManager.h"

WebDashboardManager::WebDashboardManager() : server(80)
{
    m_logger = nullptr;
    m_light = nullptr;
    m_power = nullptr;
    m_temp = nullptr;
    m_fan = nullptr;
    m_mutex = nullptr;
}

void WebDashboardManager::begin(LogManager *sysLogger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, SemaphoreHandle_t *mutex, const String &fwVer)
{
    m_logger = sysLogger;
    m_light = light;
    m_power = power;
    m_temp = temp;
    m_fan = fan;
    m_mutex = mutex;
    m_fw = fwVer;

    server.on("/", HTTP_GET, [this]() {
        server.send_P(200, "text/html", index_html); 
    });
    // Create Routing
    server.on("/lighton", std::bind(&WebDashboardManager::handleManOn, this));
    server.on("/lightoff", std::bind(&WebDashboardManager::handleManOff, this));
    server.on("/status", std::bind(&WebDashboardManager::handleStatus, this));
    server.on("/set_sch", std::bind(&WebDashboardManager::handleUpdateSchedule, this));
    server.on("/set_fan", std::bind(&WebDashboardManager::handleSetFan, this));
    server.on("/log", HTTP_GET, [this]() {
        if (m_logger != nullptr) {
            String logData = m_logger->getTailLogs(1000);
            
            server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            server.send(200, "text/html", ""); 
            server.sendContent("<html><body style='background:#0f172a;color:#10b981;font-family:monospace;padding:20px;white-space:pre-wrap;'>");
            server.sendContent(logData);
            server.sendContent("</body></html>");
            server.sendContent("");
        } else {
            server.send(500, "text/plain", "Logger not initialized");
        } 
    });

    server.begin();
    if (m_logger != nullptr)
    {
        m_logger->sysLog("WEB", "Local Dashboard Started on Port 80");
    }
}

void WebDashboardManager::handle()
{
    server.handleClient();
}

void WebDashboardManager::handleManOn()
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(150)) == pdTRUE)
    {
        m_light->setManualMode(true, true);
        if (m_logger != nullptr)
            m_logger->sysLog("WEB", "Mode Changed: ON (Manual)");
        xSemaphoreGive(*m_mutex);
        server.send(200, "application/json", "{\"status\":\"success\"}");
    }
    else
    {
        server.send(503, "application/json", "{\"error\":\"System Busy\"}"); 
    }
}
void WebDashboardManager::handleManOff()
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_light->setManualMode(false, false);
        if (m_logger != nullptr)
            m_logger->sysLog("WEB", "Mode Changed: OFF (Return to AUTO)");
        xSemaphoreGive(*m_mutex);
        server.send(200, "application/json", "{\"status\":\"success\"}");
    }
    else
    {
        server.send(503, "application/json", "{\"error\":\"System Busy\"}"); 
    }
}
void WebDashboardManager::handleStatus()
{
    float v = 0.0, t_buck = 0.0;
    bool fan = false;
    int fanPct = 0;
    float fanTempStart = 38.0, fanTempMax = 45.0;
    String alertMsg = "";
    String lightStatus = "";

    int sh = 18, sm = 45, eh = 6, em = 10;
    bool schActive = false;

    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {

        uint32_t uptime = esp_timer_get_time() / 1000000;

        v = m_power->getVoltage();
        t_buck = m_temp->getBuckTemp();
        fan = m_fan->isFanRunning();
        fanPct = m_fan->getFanSpeed();
        fanTempStart = m_fan->getTempStart();
        fanTempMax = m_fan->getTempMax();
        lightStatus = m_light->isLightMode();
        schActive = m_light->getCustomScheduleActive();
        sh = m_light->getStartHour();
        sm = m_light->getStartMin();
        eh = m_light->getEndHour();
        em = m_light->getEndMin();
        if (m_pendingAlert != "")
        {
            alertMsg = m_pendingAlert;
            m_pendingAlert = "";
        }
        xSemaphoreGive(*m_mutex);

        DynamicJsonDocument doc(768);
        doc["firmware"] = m_fw;
        doc["uptime_sec"] = uptime;
        doc["volt"] = v;
        doc["temp_buck"] = isnan(t_buck) ? -1.0 : t_buck;
        doc["fan_on"] = fan;
        doc["fan_pct"] = fanPct;
        doc["temp_start"] = fanTempStart;
        doc["temp_max"] = fanTempMax;
        doc["light"] = lightStatus;

        doc["s_h"] = sh;
        doc["s_m"] = sm;
        doc["e_h"] = eh;
        doc["e_m"] = em;
        doc["sch_active"] = schActive;

        if (alertMsg != "")
        {
            doc["alert"] = alertMsg;
        }

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        server.send(200, "application/json", jsonResponse);
    }
    else
    {
        server.send(503, "application/json", "{\"error\":\"Timeout\"}");
    }
}
void WebDashboardManager::handleUpdateSchedule()
{
    if (server.hasArg("sh") && server.hasArg("sm") && server.hasArg("eh") && server.hasArg("em") && server.hasArg("active"))
    {
        int sHour = server.arg("sh").toInt();
        int sMin = server.arg("sm").toInt();
        int eHour = server.arg("eh").toInt();
        int eMin = server.arg("em").toInt();
        bool isActive = (server.arg("active") == "1" || server.arg("active") == "true");

        if (sHour < 0 || sHour > 23 || sMin < 0 || sMin > 59 ||
            eHour < 0 || eHour > 23 || eMin < 0 || eMin > 59)
        {
            server.send(400, "text/plain", "Bad Request: Time values out of range");
            return;
        }

        if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            m_light->setScheduleParams(sHour, sMin, eHour, eMin, isActive);
            xSemaphoreGive(*m_mutex);
            // NVS write and logging happen outside the mutex
            m_light->saveScheduleToPrefs();
            if (m_logger != nullptr)
            {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg), "Web Schedule Updated: %02d:%02d to %02d:%02d | Active: %d", sHour, sMin, eHour, eMin, isActive);
                m_logger->sysLog("WEB", logMsg);
            }
            server.send(200, "text/plain", "Schedule Saved Successfully");
        }
        else
        {
            server.send(503, "text/plain", "System Busy - Try Again");
        }
    }
    else
    {
        server.send(400, "text/plain", "Bad Request: Missing Time Parameters");
    }
}
void WebDashboardManager::handleSetFan()
{
    if (!server.hasArg("ts"))
    {
        server.send(400, "text/plain", "Bad Request: Missing Parameter ts");
        return;
    }

    float tempStart = server.arg("ts").toFloat();

    if (tempStart < 30.0 || tempStart > 45.0)
    {
        server.send(400, "text/plain", "Bad Request: tempStart must be 30-45 C");
        return;
    }

    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_fan->setTempStart(tempStart);
        xSemaphoreGive(*m_mutex);
        m_fan->saveFanSetupToPrefs();
        if (m_logger != nullptr)
        {
            char logMsg[64];
            snprintf(logMsg, sizeof(logMsg), "Fan Config Updated: Start=%.1fC", tempStart);
            m_logger->sysLog("WEB", logMsg);
        }
        server.send(200, "text/plain", "Fan Settings Saved");
    }
    else
    {
        server.send(503, "text/plain", "System Busy - Try Again");
    }
}

void WebDashboardManager::triggerWebAlert(const String &module, const String &message)
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_pendingAlert = "🚨 อันตราย โมดูล: " + module + " | สถานะ: " + message;
        xSemaphoreGive(*m_mutex);
    }
}
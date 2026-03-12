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

WebDashboardManager::WebDashboardManager() : server(80) {}

void WebDashboardManager::begin(LogManager *sysLogger, LightManager *light, PowerManager *power, TempManager *temp, FanManager *fan, SemaphoreHandle_t *mutex)
{
    m_logger = sysLogger;
    m_light = light;
    m_power = power;
    m_temp = temp;
    m_fan = fan;
    m_mutex = mutex;
    if (!LittleFS.begin(true)) {
        if (m_logger) m_logger->sysLog("WEB", "LittleFS Mount Failed");
        return;
    }

    server.serveStatic("/", LittleFS, "/");
    // Create Routing
    server.on("/autosetting", std::bind(&WebDashboardManager::handleautoSetting, this));
    server.on("/lighton", std::bind(&WebDashboardManager::handleManOn, this));
    server.on("/lightoff", std::bind(&WebDashboardManager::handleManOff, this));
    server.on("/status", std::bind(&WebDashboardManager::handleStatus, this));
    server.on("/set_sch", std::bind(&WebDashboardManager::handleUpdateSchedule, this));

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

void WebDashboardManager::handleautoSetting()
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_light->setManualMode(false, 0);
        if (m_logger != nullptr)
            m_logger->sysLog("WEB", "Mode Changed: AUTO");
        xSemaphoreGive(*m_mutex);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void WebDashboardManager::handleManOn()
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_light->setManualMode(true, 100);
        if (m_logger != nullptr)
            m_logger->sysLog("WEB", "Mode Changed: MANUAL 100%");
        xSemaphoreGive(*m_mutex);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void WebDashboardManager::handleManOff()
{
    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        m_light->setManualMode(false, 0);
        if (m_logger != nullptr)
            m_logger->sysLog("WEB", "Mode Changed: OFF (Return to AUTO)");
        xSemaphoreGive(*m_mutex);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void WebDashboardManager::handleStatus()
{
    float v = 0.0, t_buck = 0.0, t_led = 0.0;
    bool fan = false;
    int pct = 0;

    if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        v = m_power->getVoltage();
        t_buck = m_temp->getBuckTemp();
        t_led = m_temp->getLedTemp(); 
        fan = m_fan->isFanRunning();     
        pct = m_light->getBrightness();
        xSemaphoreGive(*m_mutex); 

        DynamicJsonDocument doc(256);
        doc["volt"] = v;
        doc["temp_buck"] = t_buck;      
        doc["temp_led"] = t_led; 
        doc["fan_on"] = fan;            
        doc["light"] = pct;
        
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
    if (server.hasArg("p") && server.hasArg("v")) {
        int period = server.arg("p").toInt();
        int percent = server.arg("v").toInt();
        if (xSemaphoreTake(*m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            
            m_light->setPeriodBrightness(period, percent); 
            
            if (m_logger != nullptr) {
                m_logger->sysLog("WEB", "Update Slot " + String(period) + " to " + String(percent) + "%");
            }
            xSemaphoreGive(*m_mutex);
            server.send(200, "text/plain", "OK"); 
        } else {
            server.send(503, "text/plain", "System Busy");
        }
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}
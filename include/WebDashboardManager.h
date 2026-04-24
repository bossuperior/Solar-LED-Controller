#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include "LogManager.h"
#include "LightManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "index_html.h"

class WebDashboardManager
{
private:
    WebServer server;
    LightManager* m_light;
    PowerManager* m_power;
    TempManager* m_temp;
    SemaphoreHandle_t* m_mutex;
    LogManager* m_logger;
    FanManager* m_fan;

    void handleManOn();
    void handleManOff();
    void handleStatus();
    void handleUpdateSchedule();
    void handleSetFan();
    String m_pendingAlert = "";
    String m_fw = "";

public:
    WebDashboardManager();
    void begin(LogManager* sysLogger, LightManager* light, PowerManager* power, TempManager* temp,FanManager* fan, SemaphoreHandle_t* mutex,const String& fwVer);
    void handle();
    void triggerWebAlert(const String& module, const String& message);
};

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

#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#include "LightManager.h"
#include "TempManager.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "FanManager.h"
#include "SystemMonitor.h"
#include "TelegramManager.h"
#include "GsheetManager.h"
#include "WebDashboardManager.h"

#ifndef PIO_UNIT_TESTING
#define WDT_TIMEOUT 20

// --- Task & Sync Handles ---
TaskHandle_t TaskHardware;
TaskHandle_t TaskComm;
SemaphoreHandle_t mutexKey;

// --- Managers ---
NetworkManager network;
TimeManager timer;
OTAManager ota;
LightManager light;
SystemMonitor monitor;
Preferences preferences;
TempManager temp;
LogManager sysLogger;
PowerManager power;
FanManager fan;
TelegramManager telegram;
GsheetManager gsheet;
WebDashboardManager dashboard;

// --- Global Shared Variables ---
String firmwareVersion;
bool hasCheckedToday = false;
const int UPDATE_HOUR = 17;
const int UPDATE_MINUTE = 0;
const int LOG_INTERVAL = 60000; // Log every 60 seconds
bool initialOtaChecked = false;

// Shared Data (Accessed via Mutex)
int sharedLightPct = 0;
unsigned long lastLogSent = 0;

void HardwareLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  for (;;)
  {
    if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(50)) == pdTRUE)
    {
      timer.handle();
      int h = timer.getHour();
      int m = timer.getMinute();

      // Power Safety Logic
      static bool powerErrorLogged = false;
      if (!power.isPowerSafe())
      {
        if (!powerErrorLogged)
        {
          light.setManualMode(true, 0);
          sysLogger.sysLog("POWER", "Power is unsafe! Lighting turned off.");
          powerErrorLogged = true;
        }
      }
      else
      {
        if (powerErrorLogged)
        {
          light.setManualMode(false, 0);
          sysLogger.sysLog("POWER", "Power restored! Returning to AUTO mode.");
          powerErrorLogged = false;
        }
      }
      if (!ota.isUpdating)
      {
        temp.update();
        light.handle(h, m, &temp, &power);
        fan.handle(&temp);
        monitor.monitor(&power, &temp, &fan, &timer, &telegram, &light);
        light.getBrightness(sharedLightPct);
      }
      xSemaphoreGive(mutexKey);
    }
    esp_task_wdt_reset();
    vTaskDelay(20 / portTICK_PERIOD_MS); // Delay to prevent watchdog reset
  }
}
void CommLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  for (;;)
  {
    network.handle();
    dashboard.handle();
    if (network.isInternetAvailable() && xSemaphoreTake(mutexKey, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      int h = timer.getHour();
      int m = timer.getMinute();

      if ((!initialOtaChecked || (h == UPDATE_HOUR && m == UPDATE_MINUTE && !hasCheckedToday)) && !ota.isUpdating)
      {
        sysLogger.sysLog("OTA", "Scheduled update check...");
        ota.checkUpdate(firmwareVersion, &sysLogger);
        initialOtaChecked = true;
        hasCheckedToday = (h == UPDATE_HOUR);
      }
      if (h != UPDATE_HOUR)
      {
        hasCheckedToday = false;
      }

      if (!ota.isUpdating)
      {
        telegram.checkMessages(&power, &temp, &fan, &light);
        if (millis() - lastLogSent >= LOG_INTERVAL)
        {
          power.printPowerInfo();
          sysLogger.sysLog("TEMP", "LED: " + String(temp.getLedTemp(), 1) + "C | Buck: " + String(temp.getBuckTemp(), 1) + "C");
          gsheet.sendData(power.getVoltage(), temp.getLedTemp(), temp.getBuckTemp(), fan.getFanSpeed(), sharedLightPct);
          lastLogSent = millis();
        }
      }
      xSemaphoreGive(mutexKey);
    }
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay to prevent watchdog reset
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  network.begin(&sysLogger);
  timer.begin(&sysLogger);
  sysLogger.sysLog("SYSTEM", "Solar LED Controller Starting...");
  temp.begin(&sysLogger);
  power.begin(&sysLogger);
  light.begin(&sysLogger);
  fan.begin(&sysLogger);
  telegram.begin(&sysLogger);
  gsheet.begin(&sysLogger, &timer);
  monitor.begin(&sysLogger);
  dashboard.begin(&sysLogger, &light, &power, &temp, &fan, &mutexKey);

  // Load Metadata
  preferences.begin("app_info", false);
  firmwareVersion = preferences.getString("fw_ver", "v0.0.0-dev");
  preferences.end();
  sysLogger.sysLog("SYSTEM", "Firmware Version: " + firmwareVersion);

  // Create Mutex & Tasks
  mutexKey = xSemaphoreCreateMutex();
  if (mutexKey != NULL)
  {
    esp_task_wdt_init(WDT_TIMEOUT, true);
    xTaskCreatePinnedToCore(HardwareLoop, "TaskHW", 8192, NULL, 3, &TaskHardware, 1);
    xTaskCreatePinnedToCore(CommLoop, "TaskComm", 10240, NULL, 1, &TaskComm, 0);
  }
}

void loop()
{
  vTaskDelete(NULL);
}

#endif
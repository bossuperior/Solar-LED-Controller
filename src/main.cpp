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
#include <nvs_flash.h>
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
#define WDT_TIMEOUT 45

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

    if (network.isInternetAvailable())
    {
      int h = 0, m = 0;
      float send_v = 0, send_led_t = 0, send_buck_t = 0;
      int send_fan = 0, send_light = 0;
      bool doOTA = false;
      bool doLog = false;
      bool canUpdateTelegram = false;

      // Mutex to safely read shared data and check conditions without blocking for too long
      if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        h = timer.getHour();
        m = timer.getMinute();

        if (!ota.isUpdating)
        {
          canUpdateTelegram = true;
          send_v = power.getVoltage();
          send_led_t = temp.getLedTemp();
          send_buck_t = temp.getBuckTemp();
          send_fan = fan.getFanSpeed();
          send_light = sharedLightPct;
        }
        if ((!initialOtaChecked || (h == UPDATE_HOUR && m == UPDATE_MINUTE && !hasCheckedToday)) && !ota.isUpdating)
        {
          doOTA = true;
          initialOtaChecked = true;
          hasCheckedToday = (h == UPDATE_HOUR);
        }
        if (h != UPDATE_HOUR)
        {
          hasCheckedToday = false;
        }
        if (!ota.isUpdating && (millis() - lastLogSent >= LOG_INTERVAL))
        {
          doLog = true;
          lastLogSent = millis();
        }
        xSemaphoreGive(mutexKey);
      }
      if (canUpdateTelegram)
      {
        telegram.checkMessages(&power, &temp, &fan, &light, &ota);
        esp_task_wdt_reset();
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      if (ota.pendingForceUpdate)
      {
        ota.pendingForceUpdate = false;
        sysLogger.sysLog("OTA", "Force Update triggered from Telegram!");
        ota.checkUpdate(firmwareVersion, &sysLogger, &power, &telegram, true);
        esp_task_wdt_reset();
      }
      if (doOTA)
      {
        sysLogger.sysLog("OTA", "Scheduled update check...");
        ota.checkUpdate(firmwareVersion, &sysLogger, &power, &telegram);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
      if (doLog)
      {
        power.printPowerInfo();
        sysLogger.sysLog("TEMP", "LED: " + String(send_led_t, 1) + "C | Buck: " + String(send_buck_t, 1) + "C");
        gsheet.sendData(send_v, send_led_t, send_buck_t, send_fan, send_light);
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    }
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  sysLogger.begin();
  if (err == ESP_OK)
  {
    sysLogger.sysLog("SYSTEM", "NVS Initialized successfully.");
  }
  network.begin(&sysLogger);
  timer.begin(&sysLogger);
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
  if (!preferences.isKey("fw_ver")) {
      preferences.putString("fw_ver", "v0.1.5");
      sysLogger.sysLog("SYSTEM", "New NVS Key created");
  }
  firmwareVersion = preferences.getString("fw_ver", "v0.1.5");
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
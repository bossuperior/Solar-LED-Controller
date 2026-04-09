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
const uint16_t IR_TX_PIN = 17;

// --- Task & Sync Handles ---
TaskHandle_t TaskHardware;
TaskHandle_t TaskComm;
SemaphoreHandle_t mutexKey;

// --- Managers ---
NetworkManager network;
TimeManager timer;
OTAManager ota;
LightManager light(IR_TX_PIN);
SystemMonitor monitor;
Preferences prefs;
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
unsigned long lastLogSent = 0;

void HardwareLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  for (;;)
  {
    if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      timer.handle();
      int h = timer.getHour();
      int m = timer.getMinute();

      temp.update();
      fan.handle(&temp);

      // Power Safety Logic
      static bool powerErrorLogged = false;
      if (!power.isPowerSafe())
      {
        if (!powerErrorLogged)
        {
          light.setManualMode(true, false);
          sysLogger.sysLog("POWER", "Power is unsafe! Lighting turned off.");
          powerErrorLogged = true;
        }
      }
      else
      {
        if (powerErrorLogged)
        {
          light.setManualMode(false, false);
          sysLogger.sysLog("POWER", "Power restored! Returning to AUTO mode.");
          powerErrorLogged = false;
        }
      }
      if (!ota.isUpdating)
      {
        
        light.handle(h, m, &temp, &power);
        monitor.monitor(&power, &temp, &fan, &timer, &telegram, &light, &dashboard);
      }
      xSemaphoreGive(mutexKey);
    }
    esp_task_wdt_reset();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Delay to prevent watchdog reset
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
      float send_v = 0, send_buck_t = 0;
      int send_fan = 0;
      String send_light = "ปิดไฟ";
      bool doOTA = false;
      bool doLog = false;

      // Mutex to safely read shared data and check conditions without blocking for too long
      if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(150)) == pdTRUE)
      {
        h = timer.getHour();
        m = timer.getMinute();

        if (!ota.isUpdating)
        {
          send_v = power.getVoltage();
          send_buck_t = temp.getBuckTemp();
          send_fan = fan.getFanSpeed();
        }
        if ((!initialOtaChecked || (h == UPDATE_HOUR && m == UPDATE_MINUTE && !hasCheckedToday)) && !ota.isUpdating)
        {
          doOTA = true;
          initialOtaChecked = true;
          hasCheckedToday = true;
        }
        else if (h != UPDATE_HOUR)
        {
          hasCheckedToday = false;
        }
        if (!ota.isUpdating && (millis() - lastLogSent >= LOG_INTERVAL))
        {
          doLog = true;
          lastLogSent = millis();
          send_light = light.isLightMode();
        }
        xSemaphoreGive(mutexKey);
      }
      if (!ota.isUpdating)
      {
        telegram.checkMessages(&power, &temp, &fan, &light, &ota);
        esp_task_wdt_reset();
      }
      if (ota.pendingForceUpdate)
      {
        ota.pendingForceUpdate = false;
        sysLogger.sysLog("OTA", "Force Update triggered from Telegram!");
        ota.checkUpdate(firmwareVersion, &sysLogger, &power, &telegram, true);
        esp_task_wdt_reset();
      }
      else if (doOTA)
      {
        sysLogger.sysLog("OTA", "Scheduled update check...");
        ota.checkUpdate(firmwareVersion, &sysLogger, &power, &telegram);
      }
      if (doLog)
      {
        power.printPowerInfo();
        sysLogger.sysLog("TEMP", "Buck: " + String(send_buck_t, 1) + "C");
        gsheet.sendData(send_v,  send_buck_t, send_fan, send_light);
        esp_task_wdt_reset();
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
  mutexKey = xSemaphoreCreateMutex();
  if (mutexKey == NULL) {
      Serial.println("System HALT: Failed to create mutex!");
      while(1); // Halt system if RTOS fails
  }
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  sysLogger.begin();
  if (err == ESP_OK)
  {
    Serial.println("[SYSTEM] NVS Initialized successfully.");
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
  prefs.begin("app_info", false);
  if (!prefs.isKey("fw_ver")) {
      prefs.putString("fw_ver", "v0.2.1");
      sysLogger.sysLog("SYSTEM", "New NVS Key created");
  }
  firmwareVersion = prefs.getString("fw_ver", "v0.2.1");
  prefs.end();
  sysLogger.sysLog("SYSTEM", "Firmware Version: " + firmwareVersion);

  // Create Tasks
    esp_task_wdt_init(WDT_TIMEOUT, true);
    xTaskCreatePinnedToCore(HardwareLoop, "TaskHW", 8192, NULL, 3, &TaskHardware, 1);
    xTaskCreatePinnedToCore(CommLoop, "TaskComm", 10240, NULL, 1, &TaskComm, 0);
}

void loop()
{
  vTaskDelete(NULL);
}

#endif
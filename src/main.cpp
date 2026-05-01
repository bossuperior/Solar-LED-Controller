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
#include <vector>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <rom/rtc.h>
#include <Update.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "LightManager.h"
#include "TempManager.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "FanManager.h"
#include "SystemMonitor.h"
#include "GsheetManager.h"
#include "WebDashboardManager.h"
#include "BlynkManager.h"
#include "OTAManager.h"

#ifndef PIO_UNIT_TESTING
#define WDT_TIMEOUT 45
const uint16_t IR_TX_PIN = 17;
RTC_NOINIT_ATTR int crashCounter;

// --- Task & Sync Handles ---
TaskHandle_t TaskHardware;
TaskHandle_t TaskComm;
SemaphoreHandle_t mutexKey;

// --- Managers ---
NetworkManager network;
TimeManager timer;
LightManager light(IR_TX_PIN);
SystemMonitor monitor;
Preferences prefs;
TempManager temp;
LogManager sysLogger;
PowerManager power;
FanManager fan;
GsheetManager gsheet;
WebDashboardManager dashboard;
BlynkManager blynk;
OTAManager ota;

// --- Global Shared Variables ---
const int LOG_INTERVAL = 60000; // Log every 60 seconds

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
      light.handle(h, m, &temp);
      monitor.monitor(&power, &temp, &fan, &timer);
      xSemaphoreGive(mutexKey);
    }
    light.executeIR();
    if (monitor.isPendingReboot())
    {
      vTaskDelay(pdMS_TO_TICKS(3000));
      ESP.restart();
    }
    // static unsigned long lastStackPrintHW = 0;
    // if (millis() - lastStackPrintHW >= 10000)
    // {
    //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
    //     Serial.print("HW Task Free Stack: ");
    //     Serial.println(stackLeft);
    //     lastStackPrintHW = millis();
    // }
    esp_task_wdt_reset();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Delay to prevent watchdog reset
  }
}
void CommLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  unsigned long lastTelemetryUpdate = 0;
  std::vector<String> pending;
  for (;;)
  {
    network.handle();
    dashboard.handle();

    if (network.isInternetAvailable())
    {
      blynk.handle();
      float send_v = 0, send_buck_t = 0;
      int send_fan = 0;
      static String send_light = "ปิดไฟ";
      bool doLog = false;
      pending.clear();

      // Mutex to safely read shared data and check conditions without blocking for too long
      if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(150)) == pdTRUE)
      {
        while (monitor.hasAlert())
        {
          String alertTxt = monitor.getAlert();
          pending.push_back(alertTxt);
        }
        send_v = power.getVoltage();
        send_buck_t = temp.getBuckTemp();
        send_fan = fan.getFanSpeed();
        if (millis() - lastLogSent >= LOG_INTERVAL)
        {
          doLog = true;
          lastLogSent = millis();
          send_light = light.isLightMode();
        }
        xSemaphoreGive(mutexKey);
      }
      for (auto &alert : pending)
      {
        blynk.sendLog(alert);
        dashboard.triggerWebAlert("SYSTEM", alert);
      }
      if (millis() - lastTelemetryUpdate >= 30000)
      {
        blynk.sendTelemetry();
        lastTelemetryUpdate = millis();
      }
      if (doLog)
      {
        power.printPowerInfo();
        char tempLogMsg[32];
        snprintf(tempLogMsg, sizeof(tempLogMsg), "Buck: %.1fC", send_buck_t);
        sysLogger.sysLog("TEMP", tempLogMsg);
        gsheet.sendData(send_v, send_buck_t, send_fan, send_light);
        esp_task_wdt_reset();
      }
    }
    // static unsigned long lastStackPrintComm = 0;
    // if (millis() - lastStackPrintComm >= 10000)
    // {
    //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
    //     Serial.print("Comm Task Free Stack: ");
    //     Serial.println(stackLeft);
    //     lastStackPrintComm = millis();
    // }
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_POWERON || reason == ESP_RST_BROWNOUT)
  {
    crashCounter = 0;
  }
  else if (reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT)
  {
    crashCounter++;
  }
  else if (reason == ESP_RST_SW)
  {
    crashCounter = 0;
  }
  if (crashCounter >= 3)
  {
    Serial.println("[CRITICAL] Bootloop Detected!");
    
    if (Update.canRollBack()) {
        Serial.println("Rolling back to previous firmware...");
        Update.rollBack();
        crashCounter = 0;
        delay(1000);
        ESP.restart();
    } else {
        Serial.println("Rollback unavailable. Erasing NVS to safe state...");
        nvs_flash_erase();
        nvs_flash_init();
        crashCounter = 0;
        delay(1000);
        ESP.restart();
    }
  }
  mutexKey = xSemaphoreCreateMutex();
  if (mutexKey == NULL)
  {
    Serial.println("System HALT: Failed to create mutex!");
    while (1)
      ; // Halt system if RTOS fails
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
  String activeVersion; 
  { 
    Preferences tempPrefs;
    tempPrefs.begin("app_info", true); 
    activeVersion = tempPrefs.getString("fw_ver", BLYNK_FIRMWARE_VERSION); 
    tempPrefs.end();
  }
  Wire.begin(); // Single I2C init — shared by DS3231 and INA226
  network.begin(&sysLogger);
  timer.begin(&sysLogger);
  temp.begin(&sysLogger);
  power.begin(&sysLogger);
  light.begin(&sysLogger);
  fan.begin(&sysLogger);
  monitor.begin(&sysLogger);
  gsheet.begin(&sysLogger, &timer);
  dashboard.begin(&sysLogger, &light, &power, &temp, &fan, &mutexKey, activeVersion);
  blynk.begin(&sysLogger, &light, &power, &temp, &fan, &timer, &mutexKey, &ota, activeVersion);

  // Create Tasks
  esp_task_wdt_init(WDT_TIMEOUT, true);
  xTaskCreatePinnedToCore(HardwareLoop, "TaskHW", 16384, NULL, 3, &TaskHardware, 1);
  xTaskCreatePinnedToCore(CommLoop, "TaskComm", 20480, NULL, 1, &TaskComm, 0);
}

void loop()
{
  vTaskDelay(portMAX_DELAY);
}

#endif
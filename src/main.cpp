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
#include <vector>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <rom/rtc.h>
#include <Update.h>
#include <Preferences.h>
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
#include "Configs.h"

#ifndef PIO_UNIT_TESTING
RTC_NOINIT_ATTR int crashCounter;
RTC_NOINIT_ATTR uint32_t crashMagic;
RTC_NOINIT_ATTR uint8_t scheduledRebootFlag;

// --- Task & Sync Handles ---
TaskHandle_t TaskHardware;
TaskHandle_t TaskComm;
TaskHandle_t TaskIR;
SemaphoreHandle_t mutexKey;
Preferences prefs;

// --- Managers ---
NetworkManager network;
TimeManager timer;
LightManager light(IR_TX_PIN);
SystemMonitor monitor;
TempManager temp;
LogManager sysLogger;
PowerManager power;
FanManager fan;
GsheetManager gsheet;
WebDashboardManager dashboard;
BlynkManager blynk;
OTAManager ota;

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
      struct tm now;
      timer.getCurrentTime(now);
      int h = now.tm_hour;
      int m = now.tm_min;
      temp.update();
      fan.handle(&temp);
      light.handle(h, m, &temp);
      monitor.monitor(&power, &temp, &fan, &timer);
      xSemaphoreGive(mutexKey);
    }
    if (monitor.isPendingReboot())
    {
      scheduledRebootFlag = 0xAB;
      vTaskDelay(pdMS_TO_TICKS(3000));
      ESP.restart();
    }
    esp_task_wdt_reset();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Delay to prevent watchdog reset
  }
}
void IRTask(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  for (;;)
  {
    light.executeIR();
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void CommLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL);
  unsigned long lastTelemetryUpdate = 0;
  static float cachedVoltage = BATT_CRITICAL_LOW_V;
  for (;;)
  {
    network.handle();
    if (cachedVoltage >= BATT_CRITICAL_LOW_V || !power.isInaAvailable())
      dashboard.handle();
    blynk.keepAlive();
    if (network.isInternetAvailable())
    {
      esp_task_wdt_reset();
      blynk.handle();
      esp_task_wdt_reset();
      float send_v = 0, send_buck_t = 0, send_chip_t = 0;
      int send_fan = 0;
      static String send_light = "ปิดไฟ";
      bool doLog = false;

      // Mutex to safely read shared data and check conditions without blocking for too long
      std::vector<String> pending;
      if (xSemaphoreTake(mutexKey, pdMS_TO_TICKS(150)) == pdTRUE)
      {
        while (monitor.hasAlert())
        {
          String alertTxt = monitor.getAlert();
          pending.push_back(alertTxt);
        }
        send_v = power.getVoltage();
        cachedVoltage = send_v;
        send_buck_t = temp.getBuckTemp();
        send_chip_t = temp.getChipTemp();
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
      if (millis() - lastTelemetryUpdate >= SEND_TELEMETRY_INTERVAL)
      {
        blynk.sendTelemetry(send_v, send_buck_t, send_chip_t, send_fan);
        lastTelemetryUpdate = millis();
      }
      if (doLog)
      {
        power.printPowerInfo();
        char tempLogMsg[32];
        snprintf(tempLogMsg, sizeof(tempLogMsg), "Buck: %.1fC", send_buck_t);
        sysLogger.sysLog("TEMP", tempLogMsg);
        if (send_v >= BATT_CRITICAL_LOW_V)
        {
          gsheet.sendData(send_v, send_buck_t, send_chip_t, send_fan, send_light);
          blynk.keepAlive();
        }
        else
        {
          sysLogger.sysLog("GSHEET", "Skip: Voltage too low for network TX");
        }
        esp_task_wdt_reset();
      }
    }
    else if (network.isWiFiConnected())
    {
      blynk.keepAlive();
    }
    esp_task_wdt_reset();
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  bool isScheduledReboot = false;
  if (crashMagic != 0xDEADBEEF)
  {
    crashCounter = 0;
    crashMagic = 0xDEADBEEF;
    scheduledRebootFlag = 0;
  }
  else if (scheduledRebootFlag == 0xAB)
  {
    isScheduledReboot = true;
    scheduledRebootFlag = 0;
  }
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
  if (crashCounter >= BOOTLOOP_CRASH_LIMIT)
  {
    Serial.println("[CRITICAL] Bootloop Detected!");

    if (Update.canRollBack())
    {
      Serial.println("Rolling back to previous firmware...");
      Update.rollBack();
      crashCounter = 0;
      delay(1000);
      ESP.restart();
    }
    else
    {
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
  {
    const char *rrStr = "Unknown";
    switch (reason)
    {
    case ESP_RST_POWERON:   rrStr = "Power-ON";    break;
    case ESP_RST_SW:        rrStr = "Software";    break;
    case ESP_RST_PANIC:     rrStr = "PANIC/Crash"; break;
    case ESP_RST_INT_WDT:   rrStr = "Int-WDT";     break;
    case ESP_RST_TASK_WDT:  rrStr = "Task-WDT";    break;
    case ESP_RST_WDT:       rrStr = "WDT";         break;
    case ESP_RST_BROWNOUT:  rrStr = "Brownout";    break;
    case ESP_RST_DEEPSLEEP: rrStr = "DeepSleep";   break;
    default: break;
    }
    char bootMsg[64];
    snprintf(bootMsg, sizeof(bootMsg), "Boot: %s | FW:%s | Crashes:%d", rrStr, FW_VERSION, crashCounter);
    sysLogger.sysLog("SYSTEM", bootMsg);
  }
  if (err == ESP_OK)
  {
    Serial.println("[SYSTEM] NVS Initialized successfully.");
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
  dashboard.begin(&sysLogger, &light, &power, &temp, &fan, &mutexKey, FW_VERSION);
  blynk.begin(&sysLogger, &light, &power, &temp, &fan, &timer, &mutexKey, &ota, FW_VERSION);
  if (isScheduledReboot)
    blynk.setScheduledReboot(true);

  // Create Tasks
  esp_task_wdt_init(WDT_TIMEOUT, true);
  xTaskCreatePinnedToCore(HardwareLoop, "TaskHW", TASK_HW_STACK_SIZE, NULL, TASK_HW_PRIORITY, &TaskHardware, TASK_HW_CORE);
  xTaskCreatePinnedToCore(IRTask, "TaskIR", TASK_IR_STACK_SIZE, NULL, TASK_IR_PRIORITY, &TaskIR, TASK_IR_CORE);
  xTaskCreatePinnedToCore(CommLoop, "TaskComm", TASK_COMM_STACK_SIZE, NULL, TASK_COMM_PRIORITY, &TaskComm, TASK_COMM_CORE);
}

void loop()
{
  vTaskDelete(NULL);
}

#endif
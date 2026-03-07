#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
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

#ifndef PIO_UNIT_TESTING
#define WDT_TIMEOUT 20

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
String firmwareVersion;

// Check for updates time setup
bool hasCheckedToday = false;
const int UPDATE_HOUR = 17;
const int UPDATE_MINUTE = 0;
const int LOG_INTERVAL = 60000; // Log every 60 seconds
bool initialOtaChecked = false;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  sysLogger.begin();
  network.begin(&sysLogger);
  sysLogger.sysLog("SYSTEM", "Solar LED Controller Starting...");
  timer.begin(&sysLogger);
  temp.begin(&sysLogger);
  power.begin(&sysLogger);
  light.begin(&sysLogger);
  fan.begin(&sysLogger);
  telegram.begin(&sysLogger);
  monitor.begin(&sysLogger);
  preferences.begin("app_info", false);
  firmwareVersion = preferences.getString("fw_ver", "v0.0.0-dev");
  preferences.end();
  sysLogger.sysLog("SYSTEM", "Firmware Version: " + firmwareVersion);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

void loop()
{
  esp_task_wdt_reset();
  network.handle();
  timer.handle();
  if (network.isInternetAvailable())
  {
    if (!initialOtaChecked && !ota.isUpdating)
    {
      ota.checkUpdate(firmwareVersion, &sysLogger);
      initialOtaChecked = true;
    }
  }
  int hourNow = timer.getHour();
  int minuteNow = timer.getMinute();
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
    powerErrorLogged = false;
  }
  if (hourNow == UPDATE_HOUR && minuteNow == UPDATE_MINUTE)
  {
    if (!hasCheckedToday && !ota.isUpdating && network.isInternetAvailable())
    {
      sysLogger.sysLog("OTA", "Scheduled update check...");
      ota.checkUpdate(firmwareVersion, &sysLogger);
      hasCheckedToday = true;
    }
  }

  if (hourNow != UPDATE_HOUR)
  {
    hasCheckedToday = false;
  }

  if (!ota.isUpdating)
  {
    temp.update();
    light.handle(hourNow, minuteNow, &temp, &power);
    fan.handle(&temp);
    monitor.monitor(&power, &temp, &fan, &timer, &telegram, &light);
    telegram.checkMessages(&power, &temp, &fan, &light);
    static unsigned long lastLogPrint = 0;
    if (millis() - lastLogPrint >= LOG_INTERVAL)
    {
      power.printPowerInfo();
      String tempMsg = "LED Temp: " + String(temp.getLedTemp(), 1) + " C, Buck Temp: " + String(temp.getBuckTemp(), 1) + " C";
      sysLogger.sysLog("TEMP", tempMsg);
      lastLogPrint = millis();
    }
  }
  delay(10);
}

#endif
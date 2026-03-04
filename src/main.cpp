#include <Arduino.h>
#include <Preferences.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#include "LightManager.h"
#include "TempManager.h"
#include "LogManager.h"
#include "PowerManager.h"
#include "FanManager.h"

NetworkManager network;
TimeManager timer;
OTAManager ota;
LightManager light;
Preferences preferences;
TempManager temp;
LogManager sysLogger;
PowerManager power;
FanManager fan;
String firmwareVersion;

// Check for updates time setup
bool hasCheckedToday = false;
const int UPDATE_HOUR = 18;
const int UPDATE_MINUTE = 0;
const int LOG_INTERVAL = 60000; // Log every 60 seconds

void setup()
{
  Serial.begin(115200);
  delay(1000);
  sysLogger.begin();
  temp.begin();
  sysLogger.sysLog("SYSTEM", "Solar LED Controller Starting...");
  power.begin(&sysLogger);
  light.begin(&sysLogger);
  fan.begin(&sysLogger);
  network.begin(&sysLogger);

  timer.begin(&sysLogger);
  preferences.begin("app_info", false);
  firmwareVersion = preferences.getString("fw_ver", "v0.0.0-dev");
  preferences.end();
  sysLogger.sysLog("SYSTEM", "Firmware Version: " + firmwareVersion);
  ota.checkUpdate(firmwareVersion, &sysLogger);
}

void loop()
{
  network.handle();
  if (network.isInternetAvailable())
  {
    timer.handle();
  }
  int hourNow = timer.getHour();
  int minuteNow = timer.getMinute();
  static bool powerErrorLogged = false; // เพิ่มตัวแปรจำสถานะ

  if (!power.isPowerSafe())
  {
    light.forceOff();
    if (!powerErrorLogged)
    { 
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
    light.handle(hourNow, minuteNow, &temp, nullptr);
    fan.handle(&temp);
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

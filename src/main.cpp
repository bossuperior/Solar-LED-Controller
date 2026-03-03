#include <Arduino.h>
#include <Preferences.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#include "LightManager.h"
#include "TempManager.h"
#include "LogManager.h"
// #include "PowerManager.h"

NetworkManager network;
TimeManager timer;
OTAManager ota;
LightManager light;
Preferences preferences;
TempManager temp;
LogManager sysLogger;
// PowerManager power;
String firmwareVersion;
String LedTempMsg;
String BatTempMsg;

// Check for updates time setup
bool hasCheckedToday = false;
const int UPDATE_HOUR = 18;
const int UPDATE_MINUTE = 0;
const int TEMP_CHECK_INTERVAL = 2000; // 2 seconds

void setup()
{
  Serial.begin(115200);
  delay(1000);
  network.begin();
  sysLogger.sysLog("SYSTEM", "Solar LED Controller Starting...");
  while (network.isInternetAvailable() == false)
  {
    network.handle();
    delay(500);
  }
  timer.begin();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.print(".");
    delay(500);
  }
  preferences.begin("app_info", false);
  firmwareVersion = preferences.getString("fw_ver", "v0.0.0-dev"); 
  preferences.end();
  sysLogger.sysLog("SYSTEM", "Firmware Version: " + firmwareVersion);
  ota.checkUpdate(firmwareVersion);
  light.begin(&sysLogger);
  temp.begin();
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
  if (hourNow == UPDATE_HOUR && minuteNow == UPDATE_MINUTE)
  {
    if (!hasCheckedToday && !ota.isUpdating && network.isInternetAvailable())
    {
      sysLogger.sysLog("OTA", "Scheduled update check...");
      ota.checkUpdate(firmwareVersion);
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
    // power.update();
    light.handle(hourNow, minuteNow, &temp, nullptr);
    static unsigned long lastTempPrint = 0;
    if(millis() - lastTempPrint >= TEMP_CHECK_INTERVAL) {
        String tempMsg = "LED Temp: " + String(temp.getLEDTemp(), 1) + " C, Bat Temp: " + String(temp.getBatteryTemp(), 1) + " C";
        sysLogger.sysLog("TEMP", tempMsg);
        lastTempPrint = millis();
    }
  }
  delay(10);
}

#include <Arduino.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#include "LightManager.h"
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v0.0.0-dev"
#endif

NetworkManager network;
TimeManager timer;
OTAManager ota;
LightManager light;

// Check for updates time setup
bool hasCheckedToday = false;
const int UPDATE_HOUR = 3;
const int UPDATE_MINUTE = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  network.begin();
  Serial.println("---Solar LED Controller Starting---");
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
  Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
  ota.checkUpdate(FIRMWARE_VERSION);
  light.begin();
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
  light.handle(hourNow, minuteNow);
  if (hourNow == UPDATE_HOUR && minuteNow == UPDATE_MINUTE)
  {
    if (!hasCheckedToday && !ota.isUpdating && network.isInternetAvailable())
    {
      Serial.println("[System] Scheduled update check...");
      ota.checkUpdate(FIRMWARE_VERSION);
      hasCheckedToday = true;
    }
  }

  if (hourNow != UPDATE_HOUR)
  {
    hasCheckedToday = false;
  }

  if (!ota.isUpdating)
  {
    // sensor.read();
  }
  delay(10);
}

#include <Arduino.h>
#include "NetworkManager.h"
#include "TimeManager.h"
#include "OTAManager.h"
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v0.0.0-dev"
#endif

NetworkManager network;
TimeManager timer;
OTAManager ota;

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
  Serial.print("[System] Waiting for NTP time sync ");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n[System] Time Synchronized!");
  Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
  ota.checkUpdate(FIRMWARE_VERSION);
}

void loop()
{
  network.handle();
  if (network.isInternetAvailable())
  {
    timer.handle();
  }

  if (timer.getHour() == UPDATE_HOUR && timer.getMinute() == UPDATE_MINUTE)
  {
    if (!hasCheckedToday && !ota.isUpdating && network.isInternetAvailable())
    {
      Serial.println("[System] Scheduled update check...");
      ota.checkUpdate(FIRMWARE_VERSION);
      hasCheckedToday = true;
    }
  }

  if (timer.getHour() != UPDATE_HOUR)
  {
    hasCheckedToday = false;
  }

  if (!ota.isUpdating)
  {
    // sensor.read();
  }
}

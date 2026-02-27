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
  Serial.println("\n--- Solar LED Controller Starting ---");
  network.begin();
  timer.begin();
  Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
  ota.checkUpdate(FIRMWARE_VERSION);
}

void loop()
{
  network.handle();
  network.isInternetAvailable();
  timer.handle();
  if (timer.getHour() == UPDATE_HOUR && timer.getMinute() == UPDATE_MINUTE)
  {
    if (!hasCheckedToday && !ota.isUpdating && network.isInternetAvailable())
    {
      Serial.println("[System] It's update time. Connecting to GitHub...");
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

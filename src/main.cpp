#include <Arduino.h>
#include "NetworkManager.h"
#include "TimeManager.h"

NetworkManager network;
TimeManager timer;

unsigned long lastLogTime = 0;
const unsigned long logInterval = 5000; //Define log interval (5 seconds)

void setup()
{
  Serial.begin(115200);
  Serial.println("\n--- Solar LED Controller Starting ---");
  network.begin();
  timer.begin();
}

void loop()
{
  network.handle();
  network.isInternetAvailable();
  timer.handle();
  // Task Scheduling Logic
}

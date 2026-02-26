#include <Arduino.h>
#include "NetworkManager.h"

NetworkManager network;

void setup() {
  Serial.begin(9600);
  network.begin();
}

void loop() {
  network.handle();
}

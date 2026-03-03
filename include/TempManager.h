#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>

class TempManager {
public:
    void begin();
    void update();
    float getLEDTemp();
    float getBatteryTemp();

private:
    float tempLED = 0.0;
    float tempBattery = 0.0;
    DeviceAddress ledAddress;
    DeviceAddress batAddress;
};
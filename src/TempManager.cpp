#include "TempManager.h"
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void TempManager::begin()
{
    sensors.begin();
    sensors.setWaitForConversion(false); // Non-blocking mode

    // สมมติฐาน: Index 0 คือ LED, Index 1 คือ แบตเตอรี่ (ต้องสลับตามหน้างานจริง)
    sensors.getAddress(ledAddress, 0);
    sensors.getAddress(batAddress, 1);

    sensors.setResolution(ledAddress, 10);
    sensors.setResolution(batAddress, 10);
    sensors.requestTemperatures();
}

void TempManager::update()
{
    static unsigned long lastReq = 0;
    if (millis() - lastReq > 2000)
    {
        tempLED = sensors.getTempC(ledAddress);
        tempBattery = sensors.getTempC(batAddress);
        sensors.requestTemperatures();
        lastReq = millis();
    }
}

float TempManager::getLEDTemp() { return tempLED; }
float TempManager::getBatteryTemp() { return tempBattery; }

bool TempManager::isSensorError()
{
    return (tempLED <= -100.0 || tempBattery <= -100.0);
}
#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_task_wdt.h>
#include "LogManager.h"

class TempManager
{
public:
    void begin(LogManager *sysLogger);
    void update();
    void setBuckTemp(float temp)
    {
        _isTesting = true;
        _testBuckTemp = temp;
    }
    float getBuckTemp();
    float getLedTemp();
    bool isSensorHealthy() { return _ledSensorOk && _buckSensorOk; }
    bool isLedSensorOk() { return _ledSensorOk; }
    bool isBuckSensorOk() { return _buckSensorOk; }

private:
    float tempLed = 0.0;
    float tempBuck = 0.0;
    DeviceAddress ledAddress;
    DeviceAddress buckAddress;
    bool _ledSensorOk = true;
    bool _buckSensorOk = true;
    LogManager *m_sysLogger;
    float _testBuckTemp = 0.0;
    bool _isTesting = false;
};
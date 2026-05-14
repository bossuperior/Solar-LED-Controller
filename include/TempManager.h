#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_task_wdt.h>
#include "LogManager.h"

class TempManager
{
private:
    OneWire oneWire;
    DallasTemperature sensors;
    float tempBuck = 0.0;
    float tempChip = 0.0;
    DeviceAddress buckAddress;
    bool _buckSensorOk = true;
    LogManager *m_sysLogger = nullptr;
    float _testBuckTemp = 0.0;
    bool _isTesting = false;

public:
    TempManager() : oneWire(TEMP_ONE_WIRE_PIN), sensors(&oneWire) {}
    void begin(LogManager *sysLogger);
    void update();
    void setBuckTemp(float temp)
    {
        _isTesting = true;
        _testBuckTemp = temp;
    }
    float getBuckTemp();
    float getChipTemp() { return tempChip; }
    bool isBuckSensorOk() { return _buckSensorOk; }
};
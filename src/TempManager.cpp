#include "TempManager.h"
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void TempManager::begin(LogManager* sysLogger)
{
    m_sysLogger = sysLogger;
    sensors.begin();
    sensors.setWaitForConversion(false); // Non-blocking mode

    // สมมติฐาน: Index 0 คือ LED, Index 1 คือ แบตเตอรี่ (ต้องสลับตามหน้างานจริง)
    sensors.getAddress(ledAddress, 0);
    sensors.getAddress(buckAddress, 1);

    sensors.setResolution(ledAddress, 10);
    sensors.setResolution(buckAddress, 10);
    sensors.requestTemperatures();
    if (m_sysLogger != nullptr) {
        m_sysLogger->sysLog("TEMP", "Temperature sensors initialized");
    }
}

float TempManager::getLedTemp()
{
    if (!_ledSensorOk)
        return 99.9;
    return tempLed;
}
float TempManager::getBuckTemp()
{
    if (_isTesting){
        return _testBuckTemp;
    }
    if (!_buckSensorOk)
        return 50.0;
    return tempBuck;
}

void TempManager::update()
{
    static unsigned long lastReq = 0;
    if (millis() - lastReq > 2000)
    {
        float rawLED = sensors.getTempC(ledAddress);
        float rawBuck = sensors.getTempC(buckAddress);

        if (rawLED != -127.0 && rawLED != 85.0)
        {
            tempLed = rawLED;
            _ledSensorOk = true;
        }
        else
        {
            _ledSensorOk = false;
        }

        if (rawBuck != -127.0 && rawBuck != 85.0)
        {
            tempBuck = rawBuck;
            _buckSensorOk = true;
        }
        else
        {
            _buckSensorOk = false;
        }

        sensors.requestTemperatures();
        lastReq = millis();
    }
}

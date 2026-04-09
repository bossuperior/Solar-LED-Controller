#include "TempManager.h"
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void TempManager::begin(LogManager *sysLogger)
{
    m_sysLogger = sysLogger;
    sensors.begin();
    sensors.setWaitForConversion(false); // Non-blocking mode
    sensors.getAddress(buckAddress, 0);// Assuming only one sensor is connected. If multiple, this needs to be adapted.
    sensors.setResolution(buckAddress, 10);
    sensors.requestTemperatures();
    if (m_sysLogger != nullptr)
    {
        m_sysLogger->sysLog("TEMP", "Temperature sensors initialized");
    }
}
float TempManager::getBuckTemp()
{
    if (_isTesting)
    {
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
        esp_task_wdt_reset();
        float rawBuck = sensors.getTempC(buckAddress);

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
        esp_task_wdt_reset();
    }
}

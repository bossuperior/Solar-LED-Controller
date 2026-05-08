#include "TempManager.h"
OneWire oneWire(TEMP_ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

void TempManager::begin(LogManager *sysLogger)
{
    m_sysLogger = sysLogger;
    sensors.begin();
    sensors.setWaitForConversion(false); // Non-blocking mode
    if (!sensors.getAddress(buckAddress, TEMP_SENSOR_INDEX))
    {
        if (m_sysLogger != nullptr)
            m_sysLogger->sysLog("TEMP", "CRITICAL: DS18B20 sensor not found on bus!");
        _buckSensorOk = false;
    }
    else
    {
        sensors.setResolution(buckAddress, TEMP_SENSOR_RESOLUTION);
        sensors.requestTemperatures();
        _buckSensorOk = true;
        if (m_sysLogger != nullptr)
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
        return NAN;
    return tempBuck;
}

void TempManager::update()
{
    static unsigned long lastReq = 0;
    if (millis() - lastReq > TEMP_UPDATE_INTERVAL)
    {
        esp_task_wdt_reset();
        float rawBuck = sensors.getTempC(buckAddress);

        if (rawBuck != DS18B20_ERR_DISCONNECT && rawBuck != DS18B20_ERR_POWER_ON)
        {
            tempBuck = rawBuck;
            _buckSensorOk = true;
        }
        else
        {
            _buckSensorOk = false;
        }

        sensors.requestTemperatures();
        tempChip = temperatureRead();
        lastReq = millis();
        esp_task_wdt_reset();
    }
}
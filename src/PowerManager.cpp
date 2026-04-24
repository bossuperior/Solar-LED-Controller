#include "PowerManager.h"

void PowerManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
    if (!ina226.init())
    {
        if (m_logger != nullptr)
            m_logger->sysLog("POWER", "INA226 not detected! Please check wiring.");
        _inaAvailable = false;
    }
    else
    {
        _inaAvailable = true;
        ina226.setAverage(INA226_AVERAGE_16);
        ina226.setMeasureMode(INA226_CONTINUOUS);
    }
}
float PowerManager::getVoltage()
{
    if (_isTesting)
    {
        return _testVolt;
    }
    if (_inaAvailable)
    {
        return ina226.getBusVoltage_V();
    }
    return 0.0;
}

void PowerManager::printPowerInfo()
{
    if (!_inaAvailable)
    {
        if (m_logger != nullptr)
            m_logger->sysLog("POWER", "INA226 not available.");
        return;
    }
    float voltage = getVoltage();
    char powerInfo[64];
    snprintf(powerInfo, sizeof(powerInfo), "Voltage: %.2f V", voltage);

    if (m_logger != nullptr)
        m_logger->sysLog("POWER", powerInfo);
}
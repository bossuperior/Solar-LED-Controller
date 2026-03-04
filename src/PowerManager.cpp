#include "PowerManager.h"

INA226_WE ina226 = INA226_WE(0x40);

void PowerManager::begin(LogManager* sysLogger) {
    m_logger = sysLogger;
    Wire.begin();
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
        ina226.setResistorRange(0.003, 20.0);
        ina226.setMeasureMode(INA226_CONTINUOUS);
        if (m_logger != nullptr)
            m_logger->sysLog("POWER", "INA226 Smart Configured: R003/20A");
    }
}
float PowerManager::getVoltage() {
    if (_inaAvailable) {
        return ina226.getBusVoltage_V();
    }
    return 0.0;
}

float PowerManager::getCurrent() {
    if (_inaAvailable) {
        return ina226.getCurrent_A();
    }
    return 0.0;
}

float PowerManager::getPower() {
    if (_inaAvailable) {
        return ina226.getBusPower();
    }
    return 0.0;
}

void PowerManager::printPowerInfo() {
    if (!_inaAvailable) {
        if (m_logger != nullptr)
            m_logger->sysLog("POWER", "INA226 not available.");
        return;
    }
    float voltage = getVoltage();
    float current = getCurrent();
    float power = getPower();
    float cRate;
    String actionLabel;
    if (current > 0.05) { 
        actionLabel = "CHG"; 
        cRate = getChargeRate();
    } 
    else if (current < -0.05) { 
        actionLabel = "DIS"; 
        cRate = getDischargeRate();
    } 
    else {
        actionLabel = "IDLE"; 
        cRate = 0.00;
    }

    String powerInfo = "V: " + String(voltage, 2) + 
                       "V, A: " + String(current, 2) + 
                       "A, " + actionLabel + ": " + String(cRate, 2) + "W";

    if (m_logger != nullptr)
        m_logger->sysLog("POWER", powerInfo);
}


bool PowerManager::isPowerSafe() {
    if (!_inaAvailable) return false;
    float v = getVoltage();
    if (v < 2.50 || v > 3.70) {
        return false; 
    }
    return true; 
}

float PowerManager::getChargeRate() {
    if (!_inaAvailable) return 0.0;
    float power = getPower();
    if (getCurrent() < 0) return 0.0;
    return power; 
}

float PowerManager::getDischargeRate() {
    if (!_inaAvailable) return 0.0;
    float power = getPower();
    if (getCurrent() > 0) return 0.0;
    return power; 
}
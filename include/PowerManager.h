#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <INA226_WE.h>
#include "LogManager.h"

class PowerManager {
    public:
        void begin(LogManager* sysLogger);
        float getVoltage();
        float getCurrent();
        float getPower();
        float getChargeRate();
        float getDischargeRate();
        void printPowerInfo();
        bool isPowerSafe();
        bool isInaAvailable() { return _inaAvailable; }
        
    private:
        LogManager *m_logger;
        INA226_WE ina226 = INA226_WE(0x40);
        bool _inaAvailable = false;
};
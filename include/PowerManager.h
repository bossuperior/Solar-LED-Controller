#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <INA226_WE.h>
#include "LogManager.h"

class PowerManager {
    public:
        void begin(LogManager* sysLogger);
        float getVoltage();
        void setVoltage(float v) { _isTesting = true; _testVolt = v; }
        void printPowerInfo();
        bool isPowerSafe();
        bool isInaAvailable() { return _inaAvailable; }
        
    private:
        LogManager *m_logger;
        INA226_WE ina226 = INA226_WE(0x40);
        bool _inaAvailable = false;
        float _testVolt = 0.0;
        bool _isTesting = false;
};
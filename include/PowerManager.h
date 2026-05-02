#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <INA226_WE.h>
#include "LogManager.h"
#include "Configs.h"

class PowerManager {
    public:
        void begin(LogManager* sysLogger);
        float getVoltage();
        void setVoltage(float v) { _isTesting = true; _testVolt = v; }
        void printPowerInfo();
        bool isInaAvailable() { return _inaAvailable; }
        
    private:
        LogManager *m_logger = nullptr;
        INA226_WE ina226 = INA226_WE(INA226_I2C_ADDRESS);
        bool _inaAvailable = false;
        float _testVolt = 0.0;
        bool _isTesting = false;
};
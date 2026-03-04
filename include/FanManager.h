#pragma once
#include <Arduino.h>
#include "TempManager.h"
#include "LogManager.h"

class FanManager {
private:
    const int fanPin = 23;   
    const int pwmChannel = 2;  
    const int pwmFreq = 25000;  // 25kHz for quieter fan operation
    const int pwmRes = 8;       // 0-255
    
    int currentSpeed = 0;
    bool isFanRunning = false;
    LogManager* m_logger = nullptr;

public:
    void begin(LogManager* sysLogger);
    void handle(TempManager* tm); 
    void setFanSpeed(int speed);
    int getFanSpeed();
};
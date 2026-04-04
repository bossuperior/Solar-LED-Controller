#include "FanManager.h"

void FanManager::begin(LogManager* sysLogger) {
    m_logger = sysLogger;
    
    ledcSetup(pwmChannel, pwmFreq, pwmRes);
    ledcAttachPin(fanPin, pwmChannel);
    setFanSpeed(0);
    
    if (m_logger != nullptr) {
        m_logger->sysLog("FAN", "Fan Manager initialized on GPIO " + String(fanPin));
    }
}

void FanManager::handle(TempManager* tm) {
    if (tm == nullptr) return;

    float buckTemp = tm->getBuckTemp();
    float ledTemp = tm->getLedTemp();
    float maxTemp = max(buckTemp, ledTemp);
    int targetSpeed = currentSpeed;

    if (isnan(maxTemp) || maxTemp < 0.0) {
        targetSpeed = 255;
        if (m_logger != nullptr && currentSpeed != 255) {
             m_logger->sysLog("FAN", "CRITICAL: Temp sensor error! Forcing Fan ON.");
        }
    }
    else if (maxTemp > 39.0) {
        targetSpeed = 255;
    } 
    else if (maxTemp < 34.0) {
        targetSpeed = 0;
    }
    // 34.1 - 38.9 default speed
    if (targetSpeed != currentSpeed) {
        setFanSpeed(targetSpeed);
        
        if (m_logger != nullptr) {
            String status = (targetSpeed > 0) ? "ON (" + String(targetSpeed) + ")" : "OFF";
            m_logger->sysLog("FAN", "Max Temp: " + String(maxTemp, 1) + "C, Fan " + status);
        }
    }
}

void FanManager::setFanSpeed(int speed) {
    currentSpeed = constrain(speed, 0, 255);
    ledcWrite(pwmChannel, currentSpeed);
}

int FanManager::getFanSpeed() {
    return map(currentSpeed, 0, 255, 0, 100);
}
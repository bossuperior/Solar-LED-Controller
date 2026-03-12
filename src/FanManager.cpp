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

    if (maxTemp > 45.0) {
        targetSpeed = 210; //210/255*100 = 82% 
    } 
    else if (maxTemp < 40.0) {
        targetSpeed = 0;
    }
    // 40.1 - 44.9 default speed
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
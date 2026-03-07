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
    int targetSpeed = currentSpeed;

    if (buckTemp > 30.0) {
        targetSpeed = 210; //210/255*100 = 82% 
        isFanRunning = true;
    } 
    else if (buckTemp < 40.0) {
        targetSpeed = 0;
        isFanRunning = false;
    }
    // 40.1 - 44.9 default speed
    if (targetSpeed != currentSpeed) {
        setFanSpeed(targetSpeed);
        
        if (m_logger != nullptr) {
            String status = (targetSpeed > 0) ? "ON (" + String(targetSpeed) + ")" : "OFF";
            m_logger->sysLog("FAN", "Buck Temp: " + String(buckTemp, 1) + "C, Fan " + status);
        }
    }
}

void FanManager::setFanSpeed(int speed) {
    currentSpeed = constrain(speed, 0, 255);
    ledcWrite(pwmChannel, currentSpeed);
}

int FanManager::getFanSpeed() {
    return currentSpeed;
}
#include "FanManager.h"

void FanManager::begin(LogManager* sysLogger) {
    m_logger = sysLogger;
    
    ledcSetup(pwmChannel, pwmFreq, pwmRes);
    ledcAttachPin(fanPin, pwmChannel);
    setFanSpeed(0);
    
    if (m_logger != nullptr) {
        char initMsg[64];
        snprintf(initMsg, sizeof(initMsg), "Fan Manager initialized on GPIO %d", fanPin);
        m_logger->sysLog("FAN", initMsg);
    }
}

void FanManager::handle(TempManager* tm) {
    if (tm == nullptr) return;

    float buckTemp = tm->getBuckTemp();
    int targetSpeed = currentSpeed;
    bool isSensorError = false;

    if (isnan(buckTemp) || buckTemp < 0.0) {
        targetSpeed = 255;
        isSensorError = true;
    }
    else if (buckTemp > 37.5) {
        targetSpeed = 255;
    } 
    else if (buckTemp < 34.0) {
        targetSpeed = 0;
    }
    // 33.1 - 37.9 default speed
    if (targetSpeed != currentSpeed) {
        setFanSpeed(targetSpeed);
        if (m_logger != nullptr) {
            if (isSensorError) {
                m_logger->sysLog("FAN", "CRITICAL: Temp sensor error! Forcing Fan ON.");
            } else {
                char logMsg[64];
                if (targetSpeed > 0) {
                    snprintf(logMsg, sizeof(logMsg), "Max Temp: %.1fC, Fan ON (%d)", buckTemp, targetSpeed);
                } else {
                    snprintf(logMsg, sizeof(logMsg), "Max Temp: %.1fC, Fan OFF", buckTemp);
                }
                m_logger->sysLog("FAN", logMsg);
            }
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
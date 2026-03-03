#include "LogManager.h"

extern TimeManager timer;

void LogManager::sysLog(String module, String message) {
    String timeNow = timer.getTimeString();
    
    Serial.printf("[%s] [%s] %s\n", timeNow.c_str(), module.c_str(), message.c_str());
}
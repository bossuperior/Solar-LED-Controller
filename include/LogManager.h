#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>
#include "TimeManager.h"
#define MAX_LOG_LINES 30   // Maximum number of log lines to keep in memory
#define LOG_ENTRY_SIZE 320 // Fits longest Thai alert (~277 bytes) with margin

class LogManager
{
private:
    char logBuffer[MAX_LOG_LINES][LOG_ENTRY_SIZE];
    int headIndex = 0;
    int currentCount = 0;
    SemaphoreHandle_t _mutex = nullptr;
public:
    void begin();
    void sysLog(const String& module,const String& message);
    String getTailLogs(int maxChars = 1000);
};
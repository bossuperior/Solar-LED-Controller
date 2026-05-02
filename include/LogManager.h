#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>
#include "TimeManager.h"
#include "Configs.h"

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
    String getTailLogs(int maxChars = LOG_DEFAULT_MAX_CHARS);
};
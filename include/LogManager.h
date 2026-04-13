#pragma once
#include <Arduino.h>
#include "TimeManager.h"
#define MAX_LOG_LINES 30 // Maximum number of log lines to keep in memory(Ram) for quick access

class LogManager
{
private:
    String logBuffer[MAX_LOG_LINES];
    int headIndex = 0;
    int currentCount = 0;
public:
    void begin();
    void sysLog(const String& module,const String& message);
    String getTailLogs(int maxChars = 1000);
};
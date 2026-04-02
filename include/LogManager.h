#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "TimeManager.h"

class LogManager
{
private:
    const char *CURRENT_LOG = "/log_current.txt";
    const char *OLD_LOG = "/log_old.txt";
    const size_t MAX_LOG_SIZE = 150 * 1024;
    void rotateLog();
public:
    void begin();
    void sysLog(String module, String message);
    String getTailLogs(int maxChars = 3000);
};
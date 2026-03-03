#pragma once
#include <Arduino.h>
#include <time.h>

class TimeManager
{
private:
    unsigned long _lastSyncMillis = 0;
    time_t _lastSyncTime = 0;
    bool _isTimeSynced = false;
    const long gmtOffset_sec = 7 * 3600;
    const int daylightOffset_sec = 0;
    const char *ntpServer = "pool.ntp.org";

public:
    void begin();
    void handle();
    void getCurrentTime(struct tm &timeinfo);
    void printTime();
    bool isSynced();
    int getHour();
    int getMinute();
    String getTimeString();
};

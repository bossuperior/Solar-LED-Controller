#pragma once
#include <Arduino.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h>

class LogManager;
class TimeManager
{
private:
    unsigned long _lastSyncMillis = 0;
    time_t _lastSyncTime = 0;
    bool _isTimeSynced = false;
    const long gmtOffset_sec = 7 * 3600;
    const int daylightOffset_sec = 0;
    const char *ntpServer = "pool.ntp.org";
    LogManager* m_logger;
    RTC_DS3231 rtc;
    bool _rtcAvailable = false;

public:
    void begin(LogManager* sysLogger);
    void handle();
    void getCurrentTime(struct tm &timeinfo);
    void printTime();
    int getHour();
    int getMinute();
    int getYear();
    String getTimeString();
};

#pragma once
#include <Arduino.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h>
#include <esp_sntp.h>

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
    LogManager* m_logger = nullptr;
    RTC_DS3231 rtc;
    bool _rtcAvailable = false;

public:
    void begin(LogManager* sysLogger);
    void handle();
    void getCurrentTime(struct tm &timeinfo);
    String getCurrentTime();
    void printTime();
    int getHour();
    int getMinute();
    int getYear();
    int getDayOfWeek();
};

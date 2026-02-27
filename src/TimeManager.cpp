#include "TimeManager.h"
#include "NetworkManager.h"

void TimeManager::begin()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov");
    Serial.println("NTP Configured. Waiting for sync...");
}

void TimeManager::handle()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        _lastSyncTime = time(nullptr);
        _lastSyncMillis = millis();
        if (!_isTimeSynced)
        {
            Serial.println("[Time] Success! System clock is now synchronized.");
            printTime();
            _isTimeSynced = true;
        }
    }
}

void TimeManager::getCurrentTime(struct tm &timeinfo)
{
    if (getLocalTime(&timeinfo))
    {
        _isTimeSynced = true;
        return;
    }
    if (_isTimeSynced)
    {
        unsigned long elapsedSeconds = (millis() - _lastSyncMillis) / 1000;
        time_t calculatedTime = _lastSyncTime + elapsedSeconds;
        struct tm *fallbackTime = localtime(&calculatedTime);
        timeinfo = *fallbackTime;
    }
    else
    {
        time_t zero = 0;
        timeinfo = *localtime(&zero);
    }
}

void TimeManager::printTime()
{
    struct tm info;
    getCurrentTime(info);
    Serial.printf("[Clock] %02d/%02d/%04d %02d:%02d:%02d (%s)\n",
                  info.tm_mday, info.tm_mon + 1, info.tm_year + 1900,
                  info.tm_hour, info.tm_min, info.tm_sec,
                  _isTimeSynced ? "Online" : "Offline-Internal");
}

int TimeManager::getHour()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo);
    return timeinfo.tm_hour;
}

int TimeManager::getMinute()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo);
    return timeinfo.tm_min;
}
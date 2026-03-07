#include "TimeManager.h"
#include "NetworkManager.h"
#include "LogManager.h"

void TimeManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
    Wire.begin();
    if (!rtc.begin())
    {
        if (m_logger != nullptr)
            m_logger->sysLog("TIME", "DS3231 not running! Please check I2C wiring or battery.");
        _rtcAvailable = false;
    }
    else
    {
        _rtcAvailable = true;
        if (m_logger != nullptr)
        {
            m_logger->sysLog("TIME", "DS3231 RTC initialized successfully.");
        }
        if (rtc.lostPower())
        {
            if (m_logger != nullptr)
                m_logger->sysLog("TIME", "DS3231 RTC lost power, please check battery.");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }

        else
        {
            DateTime now = rtc.now();
            struct tm tm_rtc;
            tm_rtc.tm_year = now.year() - 1900;
            tm_rtc.tm_mon = now.month() - 1;
            tm_rtc.tm_mday = now.day();
            tm_rtc.tm_hour = now.hour();
            tm_rtc.tm_min = now.minute();
            tm_rtc.tm_sec = now.second();

            time_t t = mktime(&tm_rtc);
            struct timeval tv = {.tv_sec = t, .tv_usec = 0};
            settimeofday(&tv, NULL);

            if (m_logger != nullptr)
                m_logger->sysLog("TIME", "System time loaded securely from DS3231.");
        }
    }
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov");
    if (m_logger != nullptr)
    {
        m_logger->sysLog("TIME", "NTP Configured. Waiting for sync...");
    }
}

void TimeManager::handle()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0))
    {
        _lastSyncTime = time(nullptr);
        _lastSyncMillis = millis();
        if (!_isTimeSynced)
        {
            if (m_logger != nullptr)
            {
                m_logger->sysLog("TIME", "Success! System clock is now synchronized.");
            }
            if (_rtcAvailable){
                rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                                    timeinfo.tm_mday, timeinfo.tm_hour,
                                    timeinfo.tm_min, timeinfo.tm_sec));

                if (m_logger != nullptr)
                    m_logger->sysLog("TIME", "DS3231 RTC automatically calibrated with NTP time.");
            }
            printTime();
            _isTimeSynced = true;
        }
    }
}

void TimeManager::getCurrentTime(struct tm &timeinfo)
{
    if (!getLocalTime(&timeinfo))
    {
        time_t zero = 0;
        timeinfo = *localtime(&zero);
        _isTimeSynced = false;
    }
    else
    {
        _isTimeSynced = true;
    }
}

void TimeManager::printTime()
{
    struct tm info;
    getCurrentTime(info);
    if (m_logger != nullptr)
    {
        m_logger->sysLog("TIME", String(info.tm_mday) + "/" + String(info.tm_mon + 1) + "/" + String(info.tm_year + 1900) + " " + String(info.tm_hour) + ":" + String(info.tm_min) + ":" + String(info.tm_sec) + " (" + (_isTimeSynced ? "Online" : "Offline-Internal") + ")");
    }
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

int TimeManager::getYear()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo);
    return timeinfo.tm_year + 1900;
}

String TimeManager::getTimeString()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo);
    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return String(timeStr);
}
/*
 * Copyright 2026 Komkrit Tungtatiyapat
 *
 * Personal and Educational Use Only.
 * This software is provided for educational and non-commercial purposes. 
 * Any commercial use, modification for commercial purposes, manufacturing, 
 * or distribution for profit is strictly prohibited without prior written 
 * permission from the author.
 * * To request a commercial license, please contact: komkrit.tungtatiyapat@gmail.com
 */

#include "TimeManager.h"
#include "NetworkManager.h"
#include "LogManager.h"

void TimeManager::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
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
            Serial.println("[TIME] DS3231 RTC initialized successfully.");
        }
        if (rtc.lostPower())
        {
            if (m_logger != nullptr)
                m_logger->sysLog("TIME", "DS3231 RTC lost power, please check battery.");
            rtc.adjust(DateTime(__DATE__, __TIME__));
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
            struct timeval tv = {.tv_sec = (t - gmtOffset_sec), .tv_usec = 0};
            settimeofday(&tv, NULL);
            _isTimeSynced = true;

            if (m_logger != nullptr)
                Serial.println("[TIME] System time loaded securely from DS3231.");
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
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            _lastSyncTime = time(nullptr);
            _lastSyncMillis = millis();

            if (_rtcAvailable)
            {
                rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                                    timeinfo.tm_mday, timeinfo.tm_hour,
                                    timeinfo.tm_min, timeinfo.tm_sec));

                if (m_logger != nullptr)
                    m_logger->sysLog("TIME", "DS3231 RTC automatically calibrated with NTP time.");
            }
            
            _isTimeSynced = true;
            printTime();
            sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
        }
    }
}

void TimeManager::getCurrentTime(struct tm &timeinfo)
{
    if (!getLocalTime(&timeinfo))
    {
        time_t zero = 0;
        localtime_r(&zero, &timeinfo);
    }
}
String TimeManager::getCurrentTime()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo); 

    if (!_isTimeSynced) {
        return "1970-01-01 00:00:00";
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    return String(timeStringBuff);
}

void TimeManager::printTime()
{
    struct tm info;
    getCurrentTime(info);
    if (m_logger != nullptr)
    {
        char timeMsg[64];
        snprintf(timeMsg, sizeof(timeMsg), "%02d/%02d/%d %02d:%02d:%02d (%s)", 
                 info.tm_mday, 
                 info.tm_mon + 1, 
                 info.tm_year + 1900, 
                 info.tm_hour, 
                 info.tm_min, 
                 info.tm_sec, 
                 _isTimeSynced ? "Online" : "Offline-Internal");
                 
        m_logger->sysLog("TIME", timeMsg);
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

int TimeManager::getDayOfWeek()
{
    struct tm timeinfo;
    getCurrentTime(timeinfo);
    return timeinfo.tm_wday;
}
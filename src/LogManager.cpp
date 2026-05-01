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

#include "LogManager.h"

extern TimeManager timer;

void LogManager::begin()
{
    _mutex = xSemaphoreCreateMutex();
    Serial.println("[SYSTEM] Log Manager initialized.");
}

void LogManager::sysLog(const String &module, const String &message)
{
    String timeNow = timer.getCurrentTime();
    char entry[LOG_ENTRY_SIZE];
    int len = snprintf(entry, sizeof(entry), "[%s] [%s] %s\n", timeNow.c_str(), module.c_str(), message.c_str());
    Serial.print(entry);

    // Zero-timeout: never block — avoids deadlock when called while holding mutexKey
    if (_mutex == nullptr || xSemaphoreTake(_mutex, 0) != pdTRUE)
        return;
    int copyLen = min(len, (int)sizeof(logBuffer[headIndex]) - 1);
    memcpy(logBuffer[headIndex], entry, copyLen);
    logBuffer[headIndex][copyLen] = '\0';
    headIndex = (headIndex + 1) % MAX_LOG_LINES;
    if (currentCount < MAX_LOG_LINES)
        currentCount++;
    xSemaphoreGive(_mutex);
}

String LogManager::getTailLogs(int maxChars)
{
    if (currentCount == 0)
        return "No logs available.";
    if (_mutex == nullptr || xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) != pdTRUE)
        return "Log busy.";

    String finalLogs;
    finalLogs.reserve(MAX_LOG_LINES * 128);
    int startIdx = (currentCount < MAX_LOG_LINES) ? 0 : headIndex;
    for (int i = 0; i < currentCount; i++)
    {
        int idx = (startIdx + i) % MAX_LOG_LINES;
        finalLogs += logBuffer[idx];
    }
    xSemaphoreGive(_mutex);

    if (finalLogs.length() > (size_t)maxChars)
    {
        finalLogs = finalLogs.substring(finalLogs.length() - maxChars);
        int firstNewLine = finalLogs.indexOf('\n');
        if (firstNewLine != -1 && firstNewLine < finalLogs.length() - 1)
        {
            finalLogs = finalLogs.substring(firstNewLine + 1);
        }
    }
    finalLogs.trim();
    return finalLogs;
}
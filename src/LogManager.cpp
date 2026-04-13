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
    Serial.println("[SYSTEM] Log Manager initialized.");
}

void LogManager::sysLog(const String &module, const String &message)
{
    String timeNow = timer.getTimeString();
    char logEntry[256];
    snprintf(logEntry, sizeof(logEntry), "[%s] [%s] %s\n", timeNow.c_str(), module.c_str(), message.c_str());
    Serial.print(logEntry);
    logBuffer[headIndex] = String(logEntry);
    headIndex = (headIndex + 1) % MAX_LOG_LINES;
    if (currentCount < MAX_LOG_LINES)
    {
        currentCount++;
    }
}

String LogManager::getTailLogs(int maxChars)
{
    if (currentCount == 0) return "📭 No logs available.";
    String finalLogs;
    finalLogs.reserve(maxChars); // Reserve extra space for safety
    int startIdx = (currentCount < MAX_LOG_LINES) ? 0 : headIndex;

    for (int i = 0; i < currentCount; i++)
    {
        int idx = (startIdx + i) % MAX_LOG_LINES;
        finalLogs += logBuffer[idx];
    }
    if (finalLogs.length() > (size_t)maxChars)
    {
        finalLogs = finalLogs.substring(finalLogs.length() - maxChars);
    }

    finalLogs.trim();
    return finalLogs;
}
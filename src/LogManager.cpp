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

void LogManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[SYSTEM] ERROR: LittleFS Mount Failed!");
        return;
    }
    Serial.println("[SYSTEM] Log Manager initialized with LittleFS.");
}

void LogManager::rotateLog() {
    Serial.println("[SYSTEM] Rotating logs... (Max size reached)");
    if (LittleFS.exists(OLD_LOG)) {
        LittleFS.remove(OLD_LOG);
    }
    if (LittleFS.exists(CURRENT_LOG)) {
        LittleFS.rename(CURRENT_LOG, OLD_LOG);
    }
}

void LogManager::sysLog(const String& module,const String& message) {
    String timeNow = timer.getTimeString();
    char logEntry[256];
    snprintf(logEntry, sizeof(logEntry), "[%s] [%s] %s\n", timeNow.c_str(), module.c_str(), message.c_str());
    Serial.print(logEntry);
    if (!LittleFS.exists(CURRENT_LOG)) {
        File tempFile = LittleFS.open(CURRENT_LOG, FILE_WRITE);
        if (tempFile) {
            tempFile.close();
        }
    }
    File file = LittleFS.open(CURRENT_LOG, FILE_APPEND);
    if (!file) {
        Serial.println("[ERROR] Failed to open log file for appending!");
        return;
    }
    file.print(logEntry);
    size_t fileSize = file.size();
    file.close();
    if (fileSize >= MAX_LOG_SIZE) {
        rotateLog();
    }
}

String LogManager::getTailLogs(int maxChars)
{
    String finalLogs;
    finalLogs.reserve(maxChars + 2); 
    
    // Check current log size first to determine how many chars we need from the old log
    size_t curFileSize = 0;
    File curFile = LittleFS.open(CURRENT_LOG, "r");
    if (curFile) {
        curFileSize = curFile.size();
    }

    int neededChars = maxChars - curFileSize;
    
    // Read from OLD_LOG first if we need more chars than what CURRENT_LOG has
    if (neededChars > 0 && LittleFS.exists(OLD_LOG)) {
        File oldFile = LittleFS.open(OLD_LOG, "r");
        if (oldFile) {
            size_t oldFileSize = oldFile.size();
            size_t seekPos = (oldFileSize > neededChars) ? (oldFileSize - neededChars) : 0;
            
            oldFile.seek(seekPos);
            if (seekPos > 0) oldFile.readStringUntil('\n'); // Skip incomplete line if we seeked into the middle of a log entry
            
            char buf[128];
            while (oldFile.available()) {
                size_t bytesRead = oldFile.read((uint8_t*)buf, sizeof(buf) - 1);
                buf[bytesRead] = '\0';
                finalLogs += buf; 
            }
            oldFile.close();
        }
    }

    // Read from CURRENT_LOG if we still have space for more chars
    if (curFile) {
        size_t seekPos = (curFileSize > maxChars) ? (curFileSize - maxChars) : 0;
        
        curFile.seek(seekPos);
        if (seekPos > 0) curFile.readStringUntil('\n'); 

        char buf[128];
        while (curFile.available()) {
            size_t bytesRead = curFile.read((uint8_t*)buf, sizeof(buf) - 1);
            buf[bytesRead] = '\0';
            finalLogs += buf; 
        }
        curFile.close();
    }

    if (finalLogs.length() == 0) {
        return "📭 ไฟล์ Log ว่างเปล่า";
    }
    
    finalLogs.trim(); 
    return finalLogs;
}
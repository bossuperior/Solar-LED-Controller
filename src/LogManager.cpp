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

void LogManager::sysLog(String module, String message) {
    String timeNow = timer.getTimeString();
    String logEntry = "[" + timeNow + "] [" + module + "] " + message + "\n";
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
    String finalLogs = "";
    size_t currentFileSize = 0;

    // Read from CURRENT_LOG first
    File curFile = LittleFS.open(CURRENT_LOG, "r");
    if (curFile)
    {
        currentFileSize = curFile.size();
        size_t seekPos = 0;
        
        if (currentFileSize > maxChars) {
            seekPos = currentFileSize - maxChars;
        }
        
        curFile.seek(seekPos);
        if (seekPos > 0) {
            curFile.readStringUntil('\n'); 
        }
        
        // Buffered read for better performance on large logs
        size_t toRead = curFile.available();
        if(toRead > 0) {
            char* buf = (char*)malloc(toRead + 1); //Reserve memory for log content + null terminator
            if(buf) {
                curFile.read((uint8_t*)buf, toRead); // Read log content into buffer
                buf[toRead] = '\0'; // Null-terminate the string
                finalLogs = String(buf);
                free(buf);
            }
        }
        curFile.close();
    }

    // Read from OLD_LOG if needed
    int neededChars = maxChars - currentFileSize;
    
    if (neededChars > 0 && LittleFS.exists(OLD_LOG))
    {
        File oldFile = LittleFS.open(OLD_LOG, "r");
        if (oldFile)
        {
            size_t oldFileSize = oldFile.size();
            size_t seekPos = 0;
            
            if (oldFileSize > neededChars) {
                seekPos = oldFileSize - neededChars;
            }
            
            oldFile.seek(seekPos);
            if (seekPos > 0) {
                oldFile.readStringUntil('\n');
            }
            size_t toRead = oldFile.available();
            if(toRead > 0) {
                char* buf = (char*)malloc(toRead + 1);
                if(buf) {
                    oldFile.read((uint8_t*)buf, toRead);
                    buf[toRead] = '\0';
                    String oldLogs = String(buf);
                    free(buf);
                    finalLogs = oldLogs + finalLogs; 
                }
            }
            oldFile.close();
        }
    }

    // Evaluate if we got any logs, if not return a message indicating the log is empty
    if (finalLogs.length() == 0)
    {
        return "📭 ไฟล์ Log ว่างเปล่า";
    }
    finalLogs.trim(); 
    return finalLogs;
}
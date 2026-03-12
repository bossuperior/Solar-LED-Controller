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
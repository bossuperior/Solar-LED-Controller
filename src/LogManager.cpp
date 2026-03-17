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
    if (!SPIFFS.begin(true)) {
        Serial.println("[SYSTEM] ERROR: SPIFFS Mount Failed!");
        return;
    }
    Serial.println("[SYSTEM] Log Manager initialized with SPIFFS.");
}

void LogManager::rotateLog() {
    Serial.println("[SYSTEM] Rotating logs... (Max size reached)");
    if (SPIFFS.exists(OLD_LOG)) {
        SPIFFS.remove(OLD_LOG);
    }
    if (SPIFFS.exists(CURRENT_LOG)) {
        SPIFFS.rename(CURRENT_LOG, OLD_LOG);
    }
}

void LogManager::sysLog(String module, String message) {
    String timeNow = timer.getTimeString();
    String logEntry = "[" + timeNow + "] [" + module + "] " + message + "\n";
    Serial.print(logEntry);
    File file = SPIFFS.open(CURRENT_LOG, FILE_APPEND);
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
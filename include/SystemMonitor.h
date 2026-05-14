#pragma once
#include <Arduino.h>
#include <queue>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "Configs.h"

class SystemMonitor {
private:
    LogManager* m_logger = nullptr;
    std::queue<String> _alertQueue;
    SemaphoreHandle_t _alertMutex = nullptr;
    unsigned long lastCheck = 0,fanStartTime = 0;
    bool errPower = false ,errTemp = false, errTime = false, errFan = false, errBuckHighTemp = false, errBuckVoltage = false, errChipTemp = false, _pendingReboot = false;

public:
    void begin(LogManager* sysLogger);
    void monitor(PowerManager* pm, TempManager* tm, FanManager* fm, TimeManager* tr);
    void addAlert(const String& module,const String& msg);
    bool hasAlert();
    String getAlert();
    void ScheduledReboot(TimeManager* tr);
    bool isPendingReboot() const { return _pendingReboot; }
};
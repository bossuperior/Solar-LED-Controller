#pragma once
#include <Arduino.h>
#include <queue>
#include "LogManager.h"
#include "PowerManager.h"
#include "TempManager.h"
#include "FanManager.h"
#include "TimeManager.h"
#include "NetworkManager.h"

class SystemMonitor {
private:
    LogManager* m_logger = nullptr;
    std::queue<String> _alertQueue;
    unsigned long lastCheck = 0;
    const unsigned long CHECK_INTERVAL = 60000;
    bool errPower = false;
    bool errTemp = false;
    bool errTime = false;
    bool errFan = false;
    bool errBuckHighTemp = false;
    bool errBuckVoltage = false;
    unsigned long fanStartTime = 0;
    bool _pendingReboot = false;

public:
    void begin(LogManager* sysLogger);
    void monitor(PowerManager* pm, TempManager* tm, FanManager* fm, TimeManager* tr);
    void addAlert(const String& module,const String& msg);
    bool hasAlert();
    String getAlert();
    void ScheduledReboot(TimeManager* tr);
    bool isPendingReboot() const { return _pendingReboot; }
};
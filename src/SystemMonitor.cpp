#include "SystemMonitor.h"

void SystemMonitor::begin(LogManager* logger) {
    m_logger = logger;
}

void SystemMonitor::monitor(PowerManager* pm, TempManager* tm, FanManager* fm, TimeManager* tr, TelegramManager* tg) {
    if (millis() - lastCheck < CHECK_INTERVAL) return;
    lastCheck = millis();

    if (!pm->isInaAvailable()) {
        if (!errPower) {
            m_logger->sysLog("ALARM", "CRITICAL: Power Monitor (INA226) Offline!");
            errPower = true;
        }
    } else { errPower = false; }

    if (!tm->isSensorHealthy()) {
        if (!errTemp) {
            String sensor = !tm->isLedSensorOk() ? "LED" : "BUCK";
            m_logger->sysLog("ALARM", "HARDWARE: " + sensor + " Temp Sensor Failure!");
            errTemp = true;
        }
    } else { errTemp = false; }

    if (fm->getFanSpeed() > 0) {
        if (fanStartTime == 0) fanStartTime = millis();
        if (millis() - fanStartTime > 600000 && tm->getBuckTemp() > 55.0) {
            if (!errFan) {
                m_logger->sysLog("ALARM", "COOLING: Fan may be stuck! Temp high despite Fan ON.");
                errFan = true;
            }
        }
    } else {
        fanStartTime = 0;
        errFan = false;
    }

    if (tr->getYear() < 2024) { //if RTC lost power, it will reset to 1970 or similar.
        if (!errTime) {
            m_logger->sysLog("ALARM", "TIME: RTC lost power or hardware failure!");
            errTime = true;
        }
    } else { errTime = false; }
}
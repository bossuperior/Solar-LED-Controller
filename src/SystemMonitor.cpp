#include "SystemMonitor.h"

void SystemMonitor::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
}

void SystemMonitor::addAlert(const String &module, const String &msg)
{
    if (_alertQueue.size() >= MONITOR_MAX_ALERTS)
    {
        _alertQueue.pop();
    }
    _alertQueue.push("[" + module + "] " + msg);
    if (m_logger)
        m_logger->sysLog(module, msg);
}

bool SystemMonitor::hasAlert()
{
    return !_alertQueue.empty();
}

String SystemMonitor::getAlert()
{
    if (_alertQueue.empty())
        return "";
    String msg = _alertQueue.front();
    _alertQueue.pop();
    return msg;
}

void SystemMonitor::ScheduledReboot(TimeManager *tr)
{
    if (!_pendingReboot && tr->getDayOfWeek() == REBOOT_DAY_OF_WEEK && tr->getHour() == REBOOT_HOUR && tr->getMinute() == REBOOT_MINUTE && millis() > REBOOT_UPTIME_MIN_MS)
    {
        addAlert("SYSTEM", "⚙️ Weekly maintenance reboot in progress...");
        _pendingReboot = true;
    }
}

void SystemMonitor::monitor(PowerManager *pm, TempManager *tm, FanManager *fm, TimeManager *tr)
{
    if (!pm || !tm || !fm || !tr)
        return;
    if (millis() - lastCheck < MONITOR_CHECK_INTERVAL)
        return;
    lastCheck = millis();

    if (!pm->isInaAvailable())
    {
        if (!errPower)
        {
            addAlert("POWER", "🚨 อันตราย: ตัววัดแรงดันไม่ทำงาน (ตรวจสอบ INA226)");
            errPower = true;
        }
    }
    else
    {
        errPower = false;
    }

    if (!tm->isBuckSensorOk())
    {
        if (!errTemp)
        {
            addAlert("TEMP", "🚨 อันตราย: ตัววัดอุณหภูมิไม่ทำงาน (ตรวจสอบ DS18B20)");
            errTemp = true;
        }
    }
    else
    {
        errTemp = false;
    }

    if (fm->getFanSpeed() > 0)
    {
        if (fanStartTime == 0)
            fanStartTime = millis();
        if (millis() - fanStartTime > ALERT_FAN_CHECK_TIME && tm->getBuckTemp() > ALERT_FAN_TEMP_FAIL)
        {
            if (!errFan)
            {
                addAlert("FAN", "🚨 อันตราย: วงจรลดแรงดันร้อนมากทั้งที่เปิดพัดลมแล้ว พัดลมอาจมีปัญหา");
                errFan = true;
            }
        }
    }
    else
    {
        fanStartTime = 0;
        if (tm->getBuckTemp() < ALERT_FAN_TEMP_SAFE) 
        {
            errFan = false; 
        }
    }

    if (tr->getYear() < ALERT_RTC_MIN_YEAR)
    { // if RTC lost power, it will reset to 1970 or similar.
        if (!errTime)
        {
            addAlert("TIME", "⚠️ ระวัง: นาฬิกาถ่านหมดหรือพัง (ตรวจสอบ RTC DS3231)");
            errTime = true;
        }
    }
    else
    {
        ScheduledReboot(tr);
        errTime = false;
    }
    float vBus = pm->getVoltage();
    float bTemp = tm->getBuckTemp();
    if (isnan(bTemp))
    {
        errBuckHighTemp = false;
    }
    else if (bTemp > ALERT_TEMP_CRITICAL)
    {
        if (!errBuckHighTemp)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "⚠️ ระวัง: วงจรลดแรงดันร้อนจัดเกินไป (%.1f°C)", bTemp);
            addAlert("TEMP", msg);
            errBuckHighTemp = true;
        }
    }
    else if (bTemp < ALERT_TEMP_RECOVERY)
    {
        errBuckHighTemp = false;
    }

    if (vBus > ALERT_VOLTAGE_HIGH)
    {
        if (!errBuckVoltage)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "⚠️ ระวัง: วงจรลดแรงดันปล่อยแรงดันสูงเกินไป (%.2fV) อาจทำให้แบตเตอรี่เสียหาย", vBus);
            addAlert("POWER", msg);
            errBuckVoltage = true;
        }
    }
    else
    {
        errBuckVoltage = false;
    }

    float cTemp = tm->getChipTemp();
    if (cTemp > ALERT_CHIP_TEMP_CRITICAL)
    {
        if (!errChipTemp)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "⚠️ ระวัง: ชิป ESP32 ร้อนเกินไป (%.1f°C) ตรวจสอบการระบายอากาศ", cTemp);
            addAlert("CHIP", msg);
            errChipTemp = true;
        }
    }
    else if (cTemp < ALERT_CHIP_TEMP_RECOVERY)
    {
        errChipTemp = false;
    }
}
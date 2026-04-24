#include "SystemMonitor.h"

void SystemMonitor::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
}

void SystemMonitor::addAlert(const String &module, const String &msg)
{
    if (_alertQueue.size() >= 10)
    {
        _alertQueue.pop();
    }
    _alertQueue.push(msg);
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
    if (!_pendingReboot && tr->getDayOfWeek() == 5 && tr->getHour() == 7 && tr->getMinute() == 0 && millis() > 60000)
    {
        addAlert("SYSTEM", "⚙️ Weekly maintenance reboot in progress...");
        _pendingReboot = true;
    }
}

void SystemMonitor::monitor(PowerManager *pm, TempManager *tm, FanManager *fm, TimeManager *tr, LightManager *lm)
{
    if (!pm->isPowerSafe())
    {
        if (!powerErrorLogged)
        {
            lm->setManualMode(true, false);
            addAlert("POWER", "⚠️ อันตราย: แรงดันไฟไม่ปลอดภัย! ระบบปิดไฟเพื่อความปลอดภัย");
            powerErrorLogged = true;
        }
    }
    else
    {
        if (powerErrorLogged)
        {
            lm->setManualMode(false, false);
            addAlert("POWER", "✅ แรงดันไฟปลอดภัยแล้ว! ระบบกลับสู่โหมดอัตโนมัติ");
            powerErrorLogged = false;
        }
    }
    if (millis() - lastCheck < CHECK_INTERVAL)
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
            addAlert("TEMP", "🚨 อันตราย: ตัววัดอุณหภูมิวงจรลดแรงดันไม่ทำงาน (ตรวจสอบ DS18B20)");
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
        if (millis() - fanStartTime > 600000 && tm->getBuckTemp() > 55.0)
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
        errFan = false;
    }

    if (tr->getYear() < 2024)
    { // if RTC lost power, it will reset to 1970 or similar.
        if (!errTime)
        {
            addAlert("TIME", "⚠️ ระวัง: นาฬิกาในเครื่องถ่านหมดหรือพัง ทำให้ระบบตั้งเวลาเปิด/ปิดไฟไม่ทำงาน (ตรวจสอบ RTC DS3231)");
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
    else if (bTemp > 75.0)
    {
        if (!errBuckHighTemp)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "⚠️ ระวัง: วงจรลดแรงดันร้อนจัดเกินไป (%.1f°C)", bTemp);
            addAlert("TEMP", msg);
            errBuckHighTemp = true;
        }
    }
    else if (bTemp < 65.0)
    {
        errBuckHighTemp = false;
    }

    if (vBus > 3.8)
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
}
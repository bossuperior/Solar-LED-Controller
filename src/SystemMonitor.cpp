#include "SystemMonitor.h"

void SystemMonitor::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
}

void SystemMonitor::addAlert(String module, String msg)
{
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
            String msg = "🚨 อันตราย: ตัววัดแรงดันไม่ทำงาน (ตรวจสอบ INA226)";
            addAlert("POWER", msg);
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
            String sensor = "วงจรลดแรงดัน";
            String msg = "🚨 อันตราย: ตัววัดอุณหภูมิ " + sensor + " ไม่ทำงาน (ตรวจสอบ DS18B20)";
            addAlert("TEMP", msg);
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
                String msg = "🚨 อันตราย: วงจรลดแรงดันร้อนมากทั้งที่เปิดพัดลมแล้ว พัดลมอาจมีปัญหา";
                addAlert("FAN", msg);
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
            String msg = "⚠️ ระวัง: นาฬิกาในเครื่องถ่านหมดหรือพัง ทำให้ระบบตั้งเวลาเปิด/ปิดไฟไม่ทำงาน (ตรวจสอบ RTC DS3231)";
            addAlert("TIME", msg);
            errTime = true;
        }
    }
    else
    {
        errTime = false;
    }
    float vBus = pm->getVoltage();
    float bTemp = tm->getBuckTemp();
    if (bTemp > 75.0)
    {
        if (!errBuckHighTemp)
        {
            String msg = "⚠️ ระวัง: วงจรลดแรงดันร้อนจัดเกินไป (" + String(bTemp, 1) + "°C)";
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
            String msg = "⚠️ ระวัง: วงจรลดแรงดันปล่อยแรงดันสูงเกินไป (" + String(vBus, 2) + "V) อาจทำให้แบตเตอรี่เสียหาย";
            addAlert("POWER", msg);
            errBuckVoltage = true;
        }
    }
    else
    {
        errBuckVoltage = false;
    }
}
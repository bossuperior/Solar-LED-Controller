#include "SystemMonitor.h"

void SystemMonitor::begin(LogManager *sysLogger)
{
    m_logger = sysLogger;
}

void SystemMonitor::monitor(PowerManager *pm, TempManager *tm, FanManager *fm, TimeManager *tr, TelegramManager *tg, LightManager *lm)
{
    if (millis() - lastCheck < CHECK_INTERVAL)
        return;
    lastCheck = millis();

    if (!pm->isInaAvailable())
    {
        if (!errPower)
        {
            String msg = "🚨 อันตราย: เครื่องวัดไฟไม่ทำงานมองไม่เห็นแบตเตอรี่ (ตรวจสอบ INA226)";
            m_logger->sysLog("ALARM", msg);
            tg->sendAlert("POWER", msg);
            errPower = true;
        }
    }
    else
    {
        errPower = false;
    }
    if (!pm->isInaAvailable() && tr->getYear() < 2024)
    {
        if (!errBuckBoost)
        {
            String msg = "⚠️ ระวัง: ระบบไฟฟ้าไม่เสถียร! (ตรวจสอบ TPS63020)";
            m_logger->sysLog("CRITICAL", msg);
            tg->sendAlert("HARDWARE", msg);
            errBuckBoost = true;
        }
    }
    else
    {
        errBuckBoost = false;
    }

    if (!tm->isSensorHealthy())
    {
        if (!errTemp)
        {
            String sensor = !tm->isLedSensorOk() ? "ใต้แผงไฟ" : "เครื่องลดแรงดัน";
            String msg = "🚨 อันตราย: ตัววัดอุณหภูมิ " + sensor + " ไม่ทำงาน (ตรวจสอบ DS18B20)";
            m_logger->sysLog("ALARM", msg);
            tg->sendAlert("TEMP", msg);
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
                String msg = "🚨 อันตราย: เครื่องร้อนมากทั้งที่เปิดพัดลมแล้ว พัดลมอาจมีปัญหา";
                m_logger->sysLog("ALARM", msg);
                tg->sendAlert("FAN", msg);
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
            m_logger->sysLog("ALARM", msg);
            tg->sendAlert("TIME", msg);
            errTime = true;
        }
    }
    else
    {
        errTime = false;
    }
    float vBus = pm->getVoltage();
    float cCharge = pm->getCurrent();
    int currentBrightness = 0;
    lm->getBrightness(currentBrightness);
    float currentLoad = pm->getCurrent();
    if (currentBrightness == 0 && currentLoad > 0.10)
    {
        if (!errMosfetShort)
        {
            tg->sendAlert("HARDWARE", "🚨 อันตราย: ไฟรั่วกินกระแสขณะปิดไฟ (ตรวจสอบ MOSFET)");
            errMosfetShort = true;
        }
    }
    else
    {
        errMosfetShort = false;
    }
    if (currentBrightness > 80 && currentLoad < 0.02)
    {
        if (!errMosfetOpen)
        {
            tg->sendAlert("HARDWARE", "⚠️ ระวัง: ไม่มีกระแสขณะเปิดไฟ (ตรวจสอบ MOSFET)");
            errMosfetOpen = true;
        }
    }
    else
    {
        errMosfetOpen = false;
    }
    float bTemp = tm->getBuckTemp(); 
    float vBat = pm->getVoltage(); 
    if (bTemp > 75.0) {
        if (!errBuckHighTemp) {
            String msg = "⚠️ ระวัง: เครื่องแปลงไฟร้อนจัดเกินไป (" + String(bTemp, 1) + "°C) ระบบกำลังลดไฟเพื่อป้องกันไฟไหม้";
            m_logger->sysLog("ALARM", msg);
            tg->sendAlert("HARDWARE", msg);
            errBuckHighTemp = true;
        }
    } else { errBuckHighTemp = false; }

    if (vBat > 3.8) {
        if (!errBuckVoltage) {
            String msg = "⚠️ ระวัง: เครื่องแปลงไฟแรงดันสูงเกินไป (" + String(vBat, 2) + "V) อาจทำให้แบตเตอรี่เสียหาย";
            m_logger->sysLog("CRITICAL", msg);
            tg->sendAlert("POWER", msg);
            errBuckVoltage = true;
        }
    } else { errBuckVoltage = false; }
}
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "LogManager.h"
#include "secret.h"
#include "TimeManager.h"

class GsheetManager
{
private:
    WiFiClientSecure client;
    LogManager* m_sysLogger;
    TimeManager* m_timeManager;
    String scriptUrl;
public:
    void begin(LogManager* sysLogger, TimeManager* timeManager);
    void sendData(float voltage, float tempLed, float tempBuck, int fanSpeed, int lightPct);
};
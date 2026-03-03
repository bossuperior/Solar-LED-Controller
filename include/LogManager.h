#pragma once
#include <Arduino.h>
#include "TimeManager.h"

class LogManager
{
public:
    void sysLog(String module, String message);
};
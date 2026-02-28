#pragma once
#include <Arduino.h>

class LightManager {
private:
    const int ledPin = 2; // ใช้ LED_BUILTIN จำลองแผง LED 300W ไปก่อน
    const int pwmChannel = 0;
    const int pwmFreq = 5000;
    const int pwmResolution = 8; // 0-255

public:
    void begin();
    void setBrightness(int percent);
    void handle(int currentHour , int currentMinute);
};
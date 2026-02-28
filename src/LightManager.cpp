#include "LightManager.h"

void LightManager::begin() {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    setBrightness(0); // Init with lights off
    Serial.println("[Light] Manager initialized.");
}

void LightManager::setBrightness(int percent) {
    //constrain percent to 0-100 and map to duty cycle 0-255
    percent = constrain(percent, 0, 100);
    int dutyCycle = map(percent, 0, 100, 0, 255);
    ledcWrite(pwmChannel, dutyCycle);
}

void LightManager::handle(int currentHour, int currentMinute) {
    int currentTotalMinutes = (currentHour * 60) + currentMinute;

    // Time thresholds in total minutes
    int timeEvening = (18 * 60) + 0;
    int timeNight   = (22 * 60) + 30;
    int timeMorning = (4 * 60) + 15;
    int timeDay     = (6 * 60) + 0;

    if (currentTotalMinutes >= timeEvening && currentTotalMinutes < timeNight) {
        setBrightness(80); 
    } 
    else if (currentTotalMinutes >= timeNight || currentTotalMinutes < timeMorning) {
        setBrightness(50);
    } 
    else if (currentTotalMinutes >= timeMorning && currentTotalMinutes < timeDay) {
        setBrightness(80); 
    } 
    else {
        setBrightness(0);
    }
}
#include <Arduino.h>
#include <unity.h>
#include "LightManager.h"
#include "TempManager.h"
#include "LogManager.h"
#include <Preferences.h>
#include "PowerManager.h"

LightManager testLight;
LogManager fakeLogger;
TempManager fakeTempManager;
PowerManager fakePowerManager;
Preferences preferences;
int timer = 0;

void setUp(void) {
    preferences.begin("light_cfg", false);
    testLight.begin(&fakeLogger); 
}

void tearDown(void) {
}

void test_preferences_save_and_load(void) {
    int testPeriod = 1;
    int testPercent = 75;   

    testLight.setPeriodBrightness(testPeriod, testPercent); 

    testLight.begin(&fakeLogger);
    testLight.setManualMode(true, testPercent); 
    int currentPower = 0;
    testLight.getBrightness(currentPower);
    TEST_ASSERT_EQUAL(testPercent, currentPower);
}

void test_manual_mode_priority(void) {
    testLight.setManualMode(true, 100); 
    testLight.handle(19, 0, nullptr, nullptr); 
    int resultPct = 0;
    testLight.getBrightness(resultPct); 
    TEST_ASSERT_EQUAL(100, resultPct);
}

void test_thermal_safety(void) {
    fakeTempManager.setBuckTemp(85.0);
    testLight.setManualMode(true, 100);

    testLight.handle(19, 0, &fakeTempManager, &fakePowerManager); 

    int resultPct = 0;
    testLight.getBrightness(resultPct);
    TEST_ASSERT_EQUAL(50, resultPct); 
}

void test_low_battery_safety(void) {
    fakeTempManager.setBuckTemp(30.0);
    fakePowerManager.setVoltage(2.80);
    testLight.setManualMode(false, 100); 

    testLight.handle(19, 0, &fakeTempManager, &fakePowerManager); 

    int resultPct = 0;
    testLight.getBrightness(resultPct);
    TEST_ASSERT_EQUAL(10, resultPct);
}
void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_preferences_save_and_load);
    RUN_TEST(test_manual_mode_priority);
    RUN_TEST(test_thermal_safety);
    RUN_TEST(test_low_battery_safety);
    UNITY_END();
}

void loop() {}
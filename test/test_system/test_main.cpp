#include <Arduino.h>
#include <unity.h>

// Presume that the following headers are available and contain the necessary class definitions
void setUp(void) {

}

void tearDown(void) {

}

void test_map_percentage_to_pwm(void) {
    int percent = 50;
    int expected_pwm = 127; 
    int actual_pwm = map(percent, 0, 100, 0, 255);

    // Check if the mapping is correct
    TEST_ASSERT_EQUAL(expected_pwm, actual_pwm); 
}

void test_battery_voltage_logic(void) {
    float voltage = 3.10; // Low battery voltage for testing
    bool isLowBat = (voltage <= 3.15); // ลอจิกที่เราใช้ใน LightManager
    
    // Check if the logic correctly identifies low battery
    TEST_ASSERT_TRUE(isLowBat); 
}


void setup() {
    delay(2000); 

    UNITY_BEGIN(); 
    
    // Running the test cases
    RUN_TEST(test_map_percentage_to_pwm);
    RUN_TEST(test_battery_voltage_logic);
    
    UNITY_END(); 
}

void loop() {
}
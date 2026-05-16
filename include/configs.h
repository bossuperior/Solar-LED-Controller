#pragma once

// ========  SYSTEM CONFIGURATION ===========
#define FW_VERSION        "0.2.7.8"
#define WDT_TIMEOUT       45
#define LOG_INTERVAL      300000
#define SERIAL_BAUD_RATE        115200
#define BOOTLOOP_CRASH_LIMIT    3
#define SEND_TELEMETRY_INTERVAL      60000
// - Hardware Task: High priority for sensor reading and control
#define TASK_HW_STACK_SIZE      12288
#define TASK_HW_PRIORITY        3
#define TASK_HW_CORE            1
// - IR Task: Medium priority, separate from HardwareLoop to avoid blocking
#define TASK_IR_STACK_SIZE      3072
#define TASK_IR_PRIORITY        2
#define TASK_IR_CORE            1
// - Communication Task: Lower priority for network and UI updates
#define TASK_COMM_STACK_SIZE    24576
#define TASK_COMM_PRIORITY      1
#define TASK_COMM_CORE          0

// // ========  HARDWARE PINS & ADDRESSES ===========
#define IR_TX_PIN               17
#define FAN_PIN                 12
#define TEMP_ONE_WIRE_PIN       4
#define INA226_I2C_ADDRESS      0x40

// // ========  FAN & PWM SETTINGS ========
#define FAN_PWM_FREQ            25000    // 25kHz for quieter fan
#define FAN_PWM_RES             8        // 8bits = 0-255
#define FAN_PWM_CHANNEL         0
// Fan Thresholds (Default Values)
#define FAN_DEFAULT_TEMP_START  38.0f
#define FAN_DEFAULT_TEMP_MAX    48.0f
#define FAN_DEFAULT_MIN_SPEED   100
#define FAN_DEFAULT_MAX_SPEED   255
#define FAN_HYSTERESIS          2.0f
#define FAN_UPDATE_TOLERANCE    10
#define FAN_MIN_TEMP_LIMIT      30.0f

// //  ========  LIGHT & IR SETTINGS (NEC) ========
#define LIGHT_STIM_INTERVAL_MS  (2UL * 3600 * 1000) // resend every 2 hours
#define LIGHT_STIM_CUTOFF_HOUR  12                   // stimulate only when hour >= 12 (evening before midnight)
#define LIGHT_STIM_4AM_HOUR     4                    // one extra stimulate at 4:00 AM
#define IR_BITS                 32
#define IR_CODE_ON              0xFFC23D
#define IR_CODE_OFF             0xFFB04F
// #define IR_CODE_FULL         0xFF10EF
#define IR_CODE_SEMI            0xFF5AA5
// #define IR_CODE_3H           0xFF22DD
// #define IR_CODE_5H           0xFFA857
// #define IR_CODE_8H           0xFF6897
//  Default Auto Schedule
#define LIGHT_DEFAULT_START_H   18
#define LIGHT_DEFAULT_START_M   45
#define LIGHT_DEFAULT_END_H     6
#define LIGHT_DEFAULT_END_M     10

// //  ========  TEMPERATURE SENSOR SETTINGS  ========
// DS18B20 Settings (Buck Sensor)
#define TEMP_SENSOR_INDEX  0 // Buck Sensor
#define TEMP_SENSOR_RESOLUTION  10
#define TEMP_UPDATE_INTERVAL    2000
// Standard Error Values
#define DS18B20_ERR_DISCONNECT  -127.0f
#define DS18B20_ERR_POWER_ON    85.0f
#define TEMP_THROTTLE_ON  45.0f
#define TEMP_THROTTLE_OFF 40.0f
// Chip (Internal) Temperature Alerts
#define ALERT_CHIP_TEMP_WARNING   70.0f
#define ALERT_CHIP_TEMP_CRITICAL  85.0f
#define ALERT_CHIP_TEMP_RECOVERY  75.0f
#define BLYNK_DELTA_CHIP_TEMP     0.5f

// //  ========  NETWORK SETTINGS ========
#define NET_CHECK_INTERVAL      120000
#define INTERNET_CHECK_INTERVAL  300000
#define WIFI_MAX_ATTEMPT_TIME   15000
#define MIN_WIFI_RSSI     -85

// //  ========  GOOGLE SHEETS API SETTINGS ========
#define GSHEET_HTTP_TIMEOUT     8000
#define GSHEET_JSON_BUFFER_SIZE 768 //Bytes
#define GSHEET_HANDSHAKE_TIMEOUT 8

// //  ========  OTA UPDATE SETTINGS ========
#define OTA_HTTP_TIMEOUT        10000 
#define OTA_DELAY_TIME          2000
#define OTA_USER_AGENT          "ESP32-OTA"
#define OTA_MIN_VALID_VOLTAGE   0.1f

// //  ========  TIME & NTP SETTINGS ========
#define NTP_SERVER_1              "pool.ntp.org"
#define NTP_SERVER_2              "time.nist.gov"
#define NTP_GMT_OFFSET_SEC      (7 * 3600)  // GMT+7
#define NTP_DAYLIGHT_OFFSET_SEC 0

// //  ========  LOG SETTINGS ========
#define MAX_LOG_LINES     30 // Maximum number of log lines to keep in memory
#define LOG_ENTRY_SIZE    320 // Fits longest Thai alert (~277 bytes) with margin
#define LOG_DEFAULT_MAX_CHARS   1000

// //  ========  MONITORING SETTINGS ========
#define MONITOR_CHECK_INTERVAL  60000
#define MONITOR_MAX_ALERTS      10
// Auto-Reboot Schedule
#define REBOOT_DAY_OF_WEEK      5       // 5 = Friday
#define REBOOT_HOUR             7
#define REBOOT_MINUTE           0
#define REBOOT_UPTIME_MIN_MS    60000
// Temperature Alerts
#define ALERT_TEMP_CRITICAL     75.0f
#define ALERT_TEMP_RECOVERY     65.0f
#define ALERT_FAN_TEMP_FAIL     55.0f
#define ALERT_FAN_TEMP_SAFE     50.0f
// Timing Alerts
#define ALERT_FAN_CHECK_TIME    600000
#define ALERT_RTC_MIN_YEAR      2025
// Power Alerts
#define ALERT_VOLTAGE_HIGH      3.8f
#define BATT_CRITICAL_LOW_V     3.10f

// //  ========  BLYNK SETTINGS ========
#define COLOR_NORMAL            "#00E676"
#define COLOR_WARNING           "#FFB300"
#define COLOR_CRITICAL          "#FF4444"
#define COLOR_WHITE             "#FFFFFF"
#define UI_BATT_WARN_V          3.20f 
// // Delta Thresholds
#define BLYNK_DELTA_VOLT        0.02f
#define BLYNK_DELTA_TEMP        0.2f
#define BLYNK_DELTA_RAM_KB      1
#define BLYNK_SAME_VOLT_COUNT   5
#define BLYNK_RECONNECT_INTERVAL 10000
#define BLYNK_CONNECT_TIMEOUT    500


// //  ========  NETWORK AP FALLBACK SETTINGS  ========
#define AP_SSID                 "T_SOLAR_LED_AP"

// //  ========  WEB DASHBOARD & SERVER SETTINGS  ========
#define WEB_SERVER_PORT         80
#define WEB_JSON_DOC_SIZE       768
#define WEB_LOG_MAX_LENGTH      1000
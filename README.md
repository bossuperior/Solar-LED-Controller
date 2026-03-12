# Solar-LED-Controller

<img width="4827" height="2697" alt="Solar_LED Circuit_Sketch" src="https://github.com/user-attachments/assets/7c7bde6c-bc2e-4f62-b7bc-4089253ebe0f" />

# 🌞 Solar LED Controller (ESP32 + FreeRTOS)

An industrial-grade, dual-core ESP32 controller designed for solar-powered LED lighting systems. This project utilizes FreeRTOS to strictly separate hardware control from network communications, ensuring real-time stability, safety, and uninterrupted lighting control.

## ✨ Key Features

* **🧠 Dual-Core Architecture (FreeRTOS):** * **Core 1 (Hardware):** Handles real-time PWM lighting, temperature monitoring, fan speed, and power safety.
    * **Core 0 (Network):** Handles Wi-Fi, Telegram Bot, Google Sheets logging, and OTA updates.
    * *Data synchronized safely using FreeRTOS Mutex.*
* **🔋 Power & Safety Management:** Continuously monitors battery/solar voltage. Automatically shuts down the lighting system if the power is deemed unsafe and recovers automatically when stable.
* **🌡️ Active Thermal Control:** Reads temperatures from both the LED module and Buck converter to dynamically adjust cooling fan speed via PWM.
* **💡 Smart Lighting:** Operates in AUTO mode based on real-time clock (RTC/NTP) schedules, with support for MANUAL override.
* **📱 Telegram Bot Integration:** Remote control and real-time system alerts (Overheat, Low Battery, System Status) directly to your smartphone.
* **📊 Cloud Data Logging:** Automatically pushes telemetry data (Voltage, Temp, Fan Speed, Brightness) to Google Sheets at scheduled intervals.
* **☁️ OTA Updates:** Supports Over-The-Air firmware updates for seamless maintenance without physical access to the board.
* **🛡️ Watchdog Timer (WDT):** Integrated hardware watchdog on both CPU cores to prevent system lockups.

## 🗂️ Software Architecture

The system is highly modular, managed by individual C++ classes:
* `PowerManager`: Voltage monitoring and safety bounds.
* `TempManager`: NTC thermistor / Temperature sensor readings.
* `LightManager`: LED PWM dimming and scheduling.
* `FanManager`: Active cooling control.
* `TelegramManager` & `GsheetManager`: Cloud and user interface.
* `OTAManager`: Firmware update handler.
* `SystemMonitor`: Cross-checks all managers for critical errors.
* `LogManager`: Centralized system event logger.

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP32 (e.g., ESP32 DOIT DevKit V1)
* **Sensors:** Voltage divider module, Temperature sensors (for LED & Buck)
* **Actuators:** High-power MOSFETs (for LED dimming), PWM Cooling Fan
* **Power:** Solar Charge Controller, Battery, Buck/Boost Converters

## 🚀 Getting Started (PlatformIO)

1. Clone this repository.
2. Open the project in **VS Code + PlatformIO**.
3. Ensure your `platformio.ini` is configured with the correct partition scheme to accommodate OTA and Wi-Fi features:
   ```ini
   board_build.partitions = min_spiffs.csv
# Solar-LED-Controller

<img width="4944" height="2697" alt="Solar_LED_Circuit_Smart" src="https://github.com/user-attachments/assets/e328cd19-2e8a-4bdb-95d4-b7937f000d6c" />

# 🌞 Solar LED Controller (ESP32 + FreeRTOS)

An industrial-grade, dual-core ESP32 controller designed for solar-powered LED lighting systems. This project utilizes FreeRTOS to strictly separate hardware control from network communications, ensuring real-time stability, safety, and uninterrupted lighting control.

## ✨ Key Features

- **🧠 Dual-Core Architecture (FreeRTOS):** \* **Core 1 (Hardware):** Handles real-time PWM lighting, temperature monitoring, fan speed, and power safety.
  - **Core 0 (Network):** Handles Wi-Fi, Telegram Bot, Google Sheets logging, Local Web API, and OTA updates.
  - _Data synchronized safely using FreeRTOS Mutex._
- **🌐 Local Web Dashboard:** Sleek, responsive UI built with Vite, TypeScript, and Tailwind CSS. Hosted directly on the ESP32 via LittleFS for real-time monitoring and schedule configuration without cloud dependency.
  
  <img width="1890" height="934" alt="image" src="https://github.com/user-attachments/assets/b3ad4c06-3f78-4ab6-80b1-bc2488ff0b87" />

- **📴 Offline AP Fallback:** Automatically switches to Access Point mode (`SOLAR_LED_AP`) if the primary Wi-Fi drops, ensuring the dashboard remains 100% accessible via `192.168.4.1` anywhere, anytime.
- **🔋 Power & Safety Management:** Continuously monitors battery/solar voltage. Automatically shuts down the lighting system if the power is deemed unsafe and recovers automatically when stable.
- **🌡️ Active Thermal Control:** Reads temperatures from both the LED module and Buck converter to dynamically adjust cooling fan speed via PWM based on the highest temperature (Max Temp).
- **💡 Smart Lighting:** Operates in AUTO mode based on real-time clock (RTC/NTP) schedules, with support for MANUAL override.
- **📱 Telegram Bot Integration:** Remote control and real-time system alerts (Overheat, Low Battery, System Status) directly to your smartphone. Includes quick links to the Local Dashboard.
- **📊 Cloud Data Logging:** Automatically pushes telemetry data (Voltage, Temp, Fan Speed, Brightness) to Google Sheets at scheduled intervals.
- **☁️ OTA Updates:** Supports Over-The-Air firmware updates for seamless maintenance without physical access to the board.
- **🛡️ Watchdog Timer (WDT):** Integrated hardware watchdog on both CPU cores to prevent system lockups.

## 🗂️ Software Architecture

The system is highly modular, managed by individual C++ classes:

- `PowerManager`: Voltage monitoring and safety bounds.
- `TempManager`: DS18B20 temperature sensor readings (Non-blocking).
- `LightManager`: LED PWM dimming and scheduling.
- `FanManager`: Active cooling control with hysteresis loop.
- `WebDashboardManager`: Serves the LittleFS frontend and handles asynchronous REST API requests.
- `TelegramManager` & `GsheetManager`: Cloud, notifications, and user interface.
- `OTAManager`: Firmware update handler.
- `SystemMonitor`: Cross-checks all managers for critical errors.
- `LogManager`: Centralized system event logger.

## 🛠️ Hardware Requirements

- **Microcontroller:** ESP32 (e.g., ESP32 DOIT DevKit V1)
- **Sensors:** Voltage divider module, DS18B20 Temperature sensors (for LED & Buck)
- **Actuators:** High-power MOSFETs (for LED dimming), PWM Cooling Fan
- **Power:** Solar Charge Controller, LiFePO4 Battery (e.g., 32140 1S4P 3.2V), Buck/Boost Converters

## 🚀 Getting Started (PlatformIO)

### 1. Project Setup

1. Clone this repository.
2. Open the project in **VS Code + PlatformIO**.
3. Ensure your `platformio.ini` is configured with the correct partition scheme to accommodate OTA, Wi-Fi, and LittleFS features:
```ini
board_build.partitions = min_spiffs.csv
```

### 2. Build Web Dashboard (Frontend)

The web interface is built using Vite. You must compile the frontend before uploading it to the ESP32.

1. Open the terminal and navigate to the `web` directory:
```bash
cd web
```
2. Install dependencies and build the project:

```bash
npm install
npm run build
```
(Note: The `vite.config.ts` is configured to automatically output the compiled files into the PlatformIO `data` folder).

### 3. Upload to ESP32
1. Upload Filesystem: In PlatformIO, go to Project Tasks > Platform > click `Build Filesystem Image`, followed by `Upload Filesystem Image`. This uploads the Web Dashboard to the ESP32's Flash memory.
2. Upload Firmware: Click the Upload button (Right Arrow icon) in the bottom toolbar to compile and upload the C++ firmware.

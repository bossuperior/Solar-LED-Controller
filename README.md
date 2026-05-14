#  Solar LED Controller (ESP32 + FreeRTOS + Blynk app)

![ESP32](https://img.shields.io/badge/ESP32-100000?style=for-the-badge&logo=espressif&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-20232A?style=for-the-badge&logo=rtos&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-F58220?style=for-the-badge&logo=PlatformIO&logoColor=white)
![Vite](https://img.shields.io/badge/Vite-B73BFE?style=for-the-badge&logo=vite&logoColor=FFD62E)

<img width="4944" height="2745" alt="Solar_LED_Circuit_Sketch_Full" src="https://github.com/user-attachments/assets/3605d4e0-3b79-410e-945a-fc59756e1330" />

An industrial-grade, smart solar street light control system. Built with a Dual-Core FreeRTOS architecture. This project utilizes FreeRTOS to strictly separate hardware control from network communications, ensuring real-time stability, safety, and uninterrupted lighting control.

---

##  Key Features

* ** Dual-Core Architecture (FreeRTOS):** 
  * **Core 1 (Hardware):** Handles real-time IR lighting control, temperature monitoring, PWM fan speed, and power safety logic.
  * **Core 0 (Network):** Handles Wi-Fi connectivity, Blynk IoT interactions, Google Sheets logging, Local Dashboard API, and OTA updates.
  * *Data is synchronized safely across cores using FreeRTOS Mutex.*
* **Local Web Dashboard:** Sleek, responsive UI built with Vite, TypeScript, and Tailwind CSS. Hosted directly on the ESP32 via LittleFS for real-time monitoring schedule configuration and full temperature-based fan control without cloud dependency.
  
<img width="1902" height="1080" alt="image" src="https://github.com/user-attachments/assets/83420c6a-9173-42f6-8ff6-22cf349f80d2" />

* **Offline AP Fallback:** Automatically activates a local Access Point (`T_SOLAR_LED_AP`) if primary Wi-Fi is lost, maintaining control via `192.168.4.1`
* **Power & Safety Management:** Continuously monitors LiFePO4 battery/solar voltage. Automatically turning light off if the power is deemed unsafe and recovers automatically when stable.
* **Active Thermal Control:** Reads temperatures from Buck converter to dynamically adjust cooling fan speed via PWM based on the highest temperature.
* **Smart Lighting:** Operates in AUTO mode based on real-time clock (RTC/NTP) schedules, with support for MANUAL override.
* **Blynk IoT Integration:** Full remote control and real-time monitoring via the Blynk app. Features include manual override, auto-schedule configuration, live telemetry (Voltage, Temp, Fan) integrated terminal for remote system logs and full temperature-based fan control.
<div align="center">
 <img width="300" alt="image" src="https://github.com/user-attachments/assets/b75a91e9-45eb-4ebe-acc3-014ca23edacc" />
</div>

- **Cloud Data Logging:** Automatically pushes telemetry data (Voltage, Temp, Fan Speed, Light Mode) to Google Sheets at scheduled intervals.
- **OTA Updates:** Supports Over-The-Air firmware updates with a built-in safety timer and auto-rollback for seamless maintenance without physical access to the board.
- **Watchdog Timer (WDT):** Integrated hardware watchdog on both CPU cores to prevent system lockups.
- **IR Transceiver:** Separated IR Transceiver environment function for reading and testing IR remote codes to ensure precise lighting control.

##  Software Architecture

The system is highly modular, managed by individual C++ classes separated by their domains:

**Hardware & Control**
* `PowerManager`: Voltage monitoring (INA226) and safety bounds.
* `TempManager`: DS18B20 temperature sensor readings (Non-blocking).
* `LightManager`: LED IR control dimming and scheduling logic.
* `FanManager`: Active cooling control with hysteresis loop.

**Network & Interface**
* `NetworkManager`: Network management logic for connect to the internet and recovery when internet not accessible.
* `BlynkManager`: Handles cloud dashboard sync, Delta-filtered telemetry, remote terminal logging, and Blynk.Air HTTPS OTA updates.
* `WebDashboardManager`: Serves the LittleFS frontend and handles asynchronous REST API requests.
* `GsheetManager`: Cloud telemetry data logging.
* `OTAManager`: Firmware update handler with auto-rollback safety.

**System Core**
* `SystemMonitor`: Watchdog cross-checking all managers for critical hardware errors.
* `LogManager`: Centralized system event logger.
* `TimeManager`: Time sync logic between ntp server with RTC time module (DS3231).

---

##  Hardware Requirements

* **Microcontroller(MCU):** ESP32 (e.g., ESP32 DOIT DevKit V1)
* **Sensors:** INA226 Voltage/Current monitor, DS18B20 Temperature sensors
* **Actuators:** 940nm IR LED (Transmitter), PWM Cooling Fan via MOSFET control
* **Power:** Solar Charge Controller (Buck Conveter) with one direction diode to prevent reserve current to buck, LiFePO4 Battery (e.g., 32140 1S4P 3.2V), Buck/Boost Converters (TPS63020)
* **RTC:** DS3231 RTC for accurate timekeeping during offline periods.

---

##  Getting Started (PlatformIO)

### 1. Project Setup

1. Clone this repository.
2. Open the project in **VS Code + PlatformIO**.
3. Ensure your `platformio.ini` is configured with the correct partition scheme to accommodate OTA, Wi-Fi, and LittleFS features:
```ini
board_build.partitions = min_spiffs.csv
board_build.filesystem = littlefs
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

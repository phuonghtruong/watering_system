# 🌱 Pico Smart Watering System (v3.3)

A professional-grade, automated irrigation controller built for the **Raspberry Pi Pico (RP2040)** using the C Pico SDK. This system monitors soil moisture and automatically manages a water pump while ensuring plant health and home safety through advanced timing logic and hardware-level protections.

## 🚀 Key Features

*   **Intelligent Watering:** Uses hysteresis logic (30% Dry threshold to start / 60% Wet threshold to stop) to prevent rapid pump "chatter."
*   **Hardware Watchdog:** System-level monitoring that automatically reboots the Pico within 5 seconds if the software or I2C bus hangs.
*   **Flood Protection:** A safety timeout (20s) kills the pump if moisture levels don't rise, preventing indoor flooding due to sensor displacement or pipe failure.
*   **Dual-Page LCD UI:** A 16x2 I2C display cycles every 4 seconds between:
    *   **Page 1:** Real-time moisture % and System Status.
    *   **Page 2:** Current Time (HH:MM:SS) and Internal Chip Temperature.
*   **Automatic RTC Sync:** Synchronizes the internal real-time clock to your computer's time automatically at the moment of compilation.
*   **Advanced Noise Filtering:** Implements a 20-sample non-blocking rolling average to eliminate electronic jitter from ADC readings.
*   **Smart Soak Logic:** After watering, the system enters a "Soaking" state for 15 seconds to allow water to penetrate the soil before re-evaluating moisture levels.

---

## 🛠 Hardware Requirements

| Component | Pin (GPIO) | Physical Pin | Description |
| :--- | :--- | :--- | :--- |
| **Raspberry Pi Pico** | - | - | Main Microcontroller |
| **I2C LCD (16x2)** | **GP4** (SDA), **GP5** (SCL) | Pin 6, 7 | Status and data display |
| **Moisture Sensor** | **GP26** (ADC0) | Pin 31 | Capacitive or Resistive sensor |
| **Relay Module** | **GP15** | Pin 20 | Controls the Water Pump |
| **Push Button** | **GP14** | Pin 19 | Manual override / Error reset |
| **Resistor + LED** | **GP15** | Pin 20 | Optional visual pump indicator |

---

## 🔌 Wiring Summary

1.  **I2C LCD:** Connect VCC to 5V (VBUS), GND to GND, SDA to GP4, and SCL to GP5.
2.  **Pump Relay:** Connect Signal to GP15. (Ensure your relay supports 3.3V logic).
3.  **Button:** Connect one leg to GP14 and the other leg to GND. (The code enables internal pull-up).
4.  **Moisture Sensor:** Connect VCC to 3.3V (3V3_OUT), GND to GND, and Signal to GP26.

---

## ⚙️ How It Works (State Machine)

The firmware is designed as a robust state machine to handle environmental edge cases:

*   **`SYSTEM_IDLE`**: The default state. If moisture drops below 30%, it triggers a pump cycle.
*   **`SYSTEM_WATERING_AUTO`**: The pump is active. It stops once the "Wet" threshold (60%) is reached or if the 20s safety timeout expires.
*   **`SYSTEM_WATERING_MANUAL`**: Activated by holding the physical button. Bypasses moisture logic but maintains the safety timeout.
*   **`SYSTEM_SOAKING`**: A 15-second mandatory wait after pumping. This prevents "over-watering" while water is still moving through the soil.
*   **`SYSTEM_ERROR_TIMEOUT`**: Triggered if the pump runs too long without a moisture change. Lock-out state; requires a button press to reset.
*   **`SYSTEM_ERROR_SENSOR`**: Triggered if the sensor reads < 0.5% (likely disconnected). Shuts down the pump for safety.

---

## 💻 Build and Flash

### 1. Compile-Time Sync
This project uses a specialized parser in `pico_rtc.c` to read the `__DATE__` and `__TIME__` macros. To ensure your clock is accurate, **rebuild the project** immediately before flashing.

### 2. Standard Build Process
```bash
mkdir build
cd build
cmake ..
make

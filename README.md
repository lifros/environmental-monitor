# Environmental Monitor

Environmental monitoring system with real-time display of air quality and climate parameters.

## Overview

This project implements an environmental control unit that reads and visualizes:

- **CO2** (ppm) — from SCD41
- **Temperature** (°C)
- **Humidity** (%)
- **VOC** (Volatile Organic Compounds)
- **Pressure** (hPa)

Data is shown on a touch LCD and can be used for logging or automation.

## Hardware

| Component | Description |
|-----------|-------------|
| **MCU / Display** | ESP32-C6-Touch-LCD-1.47 (ESP32-C6 + 1.47" touch LCD) |
| **CO2** | Sensirion SCD41 (CO2, temperature, humidity) |
| **Environment** | Bosch BME680 (temperature, humidity, pressure, VOC/gas) |

## Sensors

- **SCD41**: NDIR CO2, built-in temperature and humidity (used primarily for CO2).
- **BME680**: Temperature, relative humidity, barometric pressure, and VOC/gas resistance (indoor air quality).

## Software

- **IDE**: Arduino IDE (sketch `environmental-monitor.ino`).
- **Board**: ESP32-C6 — install **esp32 by Espressif Systems** in Board Manager (≥ 3.0.0).
- **Libraries** (Sketch → Include Library → Manage Libraries):
  - **Sensirion I2C SCD4x** (official SCD41/SCD40 driver)
  - **Adafruit BME680** (installs Adafruit Unified Sensor and BusIO as dependencies)
  - **Adafruit ST7735 and ST7789 Library** (integrated 172×320 display; installs Adafruit GFX)
- **Pinout**: See [PINOUT.md](PINOUT.md). I²C: SDA/SCL on right header; GPIOs in `config.h`.

### Build and upload (Arduino IDE)

1. **File → Open** and select the project folder (the one containing `environmental-monitor.ino`).
2. **Tools → Board** → **ESP32 Arduino** → **ESP32C6 Dev Module** (or the exact board if listed).
3. **Tools → Port** → select the COM port of the board.
4. **Sketch → Upload**.
5. **Tools → Serial Monitor** at 115200 baud to see sensor output.
6. Connect SCD41 and BME680 to the board I²C (SDA, SCL) and 3V3/GND. If sensors are not found, check I²C pins in `config.h` against the board schematic.

## Repository Structure

- `README.md` — this file
- `PINOUT.md` — board pinout
- `environmental-monitor.ino` — main Arduino sketch
- `config.h` — I²C pins and options
- `platformio.ini` — optional, for PlatformIO users
- `src/`, `include/` — optional PlatformIO layout (sketch uses root `.ino` and `config.h`)

## License

_(Specify license if needed.)_

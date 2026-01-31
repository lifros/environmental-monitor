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

- **IDE**: Arduino IDE. Main console sketch: folder `environmental_monitor_console`.
- **Board**: ESP32-C6 — install **esp32 by Espressif Systems** in Board Manager (≥ 3.0.0).
- **Libraries** (Sketch → Include Library → Manage Libraries):
  - **Sensirion I2C SCD4x** (official SCD41/SCD40 driver)
  - **Adafruit BME680** (installs Adafruit Unified Sensor and BusIO as dependencies)
  - **Adafruit ST7735 and ST7789 Library** (integrated 172×320 display; installs Adafruit GFX)
- **Pinout**: See [PINOUT.md](PINOUT.md). I²C: SDA/SCL on right header; GPIOs and IAQ range in each sketch’s `config.h`.

### Build and upload (Arduino IDE)

1. **File → Open** and select the sketch folder (e.g. `environmental_monitor_console` for Serial-only output).
2. **Tools → Board** → **ESP32 Arduino** → **ESP32C6 Dev Module** (or the exact board if listed).
3. **Tools → Port** → select the COM port of the board.
4. **Sketch → Upload**.
5. **Tools → Serial Monitor** at 115200 baud to see sensor output.
6. Connect SCD41 and BME680 to the board I²C (SDA, SCL) and 3V3/GND. If sensors are not found, check I²C pins in `config.h` against the board schematic.

## I2C addresses (verified with project scanner)

| Address | Device |
|---------|--------|
| **0x63** | Onboard touch |
| **0x6B** | Onboard IMU / touch |
| **0x62** | SCD41 (external, expansion header) |
| **0x77** | BME680 (external, expansion header) |

With only the board connected: 2 devices (0x63, 0x6B). With SCD41 and BME680 on the expansion header: 4 devices. The onboard LCD is SPI, not I2C.

**I2C scanner sketch**: open the `i2c_scanner` folder in Arduino IDE, upload, and open Serial Monitor at 115200. It scans SDA=18, SCL=19 and labels onboard addresses.

## Repository Structure

- `README.md` — this file
- `PINOUT.md` — board pinout
- `environmental_monitor_console/` — Serial-only sketch (SCD41 + BME680, IAQ); contains `environmental_monitor_console.ino` and `config.h` (I²C pins, `IAQ_R_MIN`, `IAQ_R_MAX`)
- `i2c_scanner/` — I2C bus scanner sketch

## License

_(Specify license if needed.)_

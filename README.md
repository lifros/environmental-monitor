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

- **SCD41**: NDIR CO2, built-in temperature and humidity (used primarily for CO2). For specs and **temperature/humidity compensation** (self-heating offset), see [docs/SCD41-specs-and-compensation.md](docs/SCD41-specs-and-compensation.md).
- **BME680**: Temperature, relative humidity, barometric pressure, and VOC/gas resistance (indoor air quality). For specs and configuration notes (oversampling order, heater, IAQ), see [docs/BME680-specs-and-notes.md](docs/BME680-specs-and-notes.md).

## Software

- **IDE**: Arduino IDE. Sketches: `environmental_monitor_console` (Serial only), `environmental_monitor_gui` (Serial + LCD).
- **Board**: ESP32-C6 — install **esp32 by Espressif Systems** in Board Manager (≥ 3.0.0).
- **Libraries** (Sketch → Include Library → Manage Libraries):
  - **Sensirion I2C SCD4x** (official SCD41/SCD40 driver)
  - **Adafruit BME680** (installs Adafruit Unified Sensor and BusIO as dependencies)
  - **Arduino_GFX_Library** (GUI sketch: 172×320 display with official board init; install from Library Manager)
  - **PubSubClient** (Nick O'Leary) or **PubSubClient3** (Holger Mueller) — required only for GUI sketch if MQTT is enabled (see [docs/HA-integration.md](docs/HA-integration.md))
- **Pinout**: See [PINOUT.md](PINOUT.md). I²C: SDA/SCL on right header. GUI sketch: TFT pins in `environmental_monitor_gui/config.h` (verify from board schematic).
- **Home Assistant**: GUI sketch supports **WiFi + MQTT** (Arduino); Zigbee requires ESP-IDF. See [docs/HA-integration.md](docs/HA-integration.md) and [environmental_monitor_zigbee_espidf/](environmental_monitor_zigbee_espidf/).

### Build and upload (Arduino IDE)

1. **File → Open** and select the sketch folder (`environmental_monitor_console` for Serial only, `environmental_monitor_gui` for LCD).
2. **Tools → Board** → **ESP32 Arduino** → **ESP32C6 Dev Module** (or the exact board if listed).
3. **Tools → Partition Scheme** → **Default 4MB with spiffs** (required if your board has 4MB flash; otherwise boot fails with "partition table exceeds flash").
4. **Tools → Port** → select the COM port of the board.
5. **Sketch → Upload**.
6. **Tools → Serial Monitor** at 115200 baud to see sensor output.
7. Connect SCD41 and BME680 to the board I²C (SDA, SCL) and 3V3/GND. If sensors are not found, check I²C pins in `config.h` against the board schematic.

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
- `docs/SCD41-specs-and-compensation.md` — SCD41 specs and T/RH compensation procedure
- `docs/BME680-specs-and-notes.md` — BME680 specs and configuration notes (oversampling, heater, IAQ)
- `environmental_monitor_console/` — Serial-only sketch (SCD41 + BME680, IAQ); `config.h` has I²C pins and IAQ range
- `environmental_monitor_gui/` — Same sensors + LCD (172×320, Arduino_GFX_Library, official init); optional WiFi+MQTT for Home Assistant; `config.h` has I²C, IAQ, TFT, and MQTT settings
- `environmental_monitor_zigbee_espidf/` — README and outline for Zigbee via ESP-IDF (Home Assistant)
- `i2c_scanner/` — I2C bus scanner sketch
- `docs/HA-integration.md` — Home Assistant via WiFi+MQTT (Arduino GUI) or Zigbee (ESP-IDF)

## License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for the full text.

# Environmental Monitor

CO2 monitor with optional temperature and humidity. Real-time display on touch LCD; optional WiFi + MQTT for Home Assistant.

## Overview

This project focuses on **CO2** (ppm) from the Sensirion SCD41. The SCD41 also provides temperature and humidity for context. Data is shown on a touch LCD and can be published to Home Assistant via MQTT.

- **CO2** (ppm) — SCD41
- **Temperature** (°C) — SCD41
- **Humidity** (%) — SCD41

## Hardware

| Component | Description |
|-----------|-------------|
| **MCU / Display** | ESP32-C6-Touch-LCD-1.47 (ESP32-C6 + 1.47" touch LCD) |
| **CO2 / T / RH** | Sensirion SCD41 (NDIR CO2, built-in T and RH) |

## Sensors

- **SCD41**: NDIR CO2 plus temperature and humidity. For full specs, **temperature/altitude compensation**, ASC/FRC calibration, and low-power modes, see [docs/SCD41-specs-and-compensation.md](docs/SCD41-specs-and-compensation.md). The GUI sketch supports optional altitude, persist settings, and low-power periodic mode via `config.h`.

## Software

- **IDE**: Arduino IDE. Sketch: `environmental_monitor_gui` (Serial + LCD, optional WiFi/MQTT).
- **Board**: ESP32-C6 — install **esp32 by Espressif Systems** in Board Manager (≥ 3.0.0).
- **Libraries** (Sketch → Include Library → Manage Libraries):
  - **Sensirion I2C SCD4x** (official SCD41/SCD40 driver)
  - **Arduino_GFX_Library** (172×320 display with official board init)
  - **PubSubClient** (Nick O'Leary) or **PubSubClient3** (Holger Mueller) — for MQTT; see [docs/HA-integration.md](docs/HA-integration.md)
- **Pinout**: See [PINOUT.md](PINOUT.md). I²C: SDA/SCL on expansion header. TFT pins in `environmental_monitor_gui/config.h`.
- **Home Assistant**: WiFi + MQTT via GUI sketch. See [docs/HA-integration.md](docs/HA-integration.md).

### Build and upload (Arduino IDE)

1. **File → Open** and select the sketch folder `environmental_monitor_gui`.
2. **Tools → Board** → **ESP32 Arduino** → **ESP32C6 Dev Module** (or the exact board if listed).
3. **Tools → Partition Scheme** → **Default 4MB with spiffs** (required if your board has 4MB flash; otherwise boot fails with "partition table exceeds flash").
4. **Tools → Port** → select the COM port of the board.
5. **Sketch → Upload**.
6. **Tools → Serial Monitor** at 115200 baud to see sensor output.
7. Connect SCD41 to the board I²C (SDA, SCL) and 3V3/GND. If the sensor is not found, check I²C pins in `config.h` against the board schematic.

**OTA (Over-The-Air)**: With WiFi and MQTT enabled, set `ENABLE_OTA 1` in `config.h`. After the first USB upload, the device appears in **Tools → Port** as *envmon_gui at 192.168.x.x*. Select that port and upload to update firmware without USB.

## I2C addresses (verified with project scanner)

| Address | Device |
|---------|--------|
| **0x63** | Onboard touch |
| **0x6B** | Onboard IMU / touch |
| **0x62** | SCD41 (external, expansion header) |

With only the board: 2 devices (0x63, 0x6B). With SCD41 on the expansion header: 3 devices. The onboard LCD is SPI, not I2C.

**I2C scanner sketch**: open the `i2c_scanner` folder in Arduino IDE, upload, and open Serial Monitor at 115200. It scans SDA=18, SCL=19 and labels onboard addresses.

## Repository Structure

- `README.md` — this file
- `PINOUT.md` — board pinout
- `docs/SCD41-specs-and-compensation.md` — SCD41 specs and T/RH compensation procedure
- `environmental_monitor_gui/` — SCD41 + LCD (172×320), optional WiFi+MQTT; `config.h` has I²C, TFT, and MQTT settings
- `i2c_scanner/` — I2C bus scanner sketch
- `docs/HA-integration.md` — Home Assistant via WiFi+MQTT

## License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for the full text.

# Environmental Monitor GUI — PlatformIO

Same CO2 monitor (SCD41 + LCD) as `environmental_monitor_gui` (Arduino), intended for PlatformIO. **OTA upload by IP** would be supported without mDNS.

## ⚠️ ESP32-C6 + Arduino on PlatformIO

**As of 2025, the PlatformIO `espressif32` platform does not enable the Arduino framework for the ESP32-C6 board.** Build fails with: *"This board doesn't support arduino framework!"*

**Until PlatformIO adds Arduino support for ESP32-C6:**

- Use **Arduino IDE** with the sketch in `environmental_monitor_gui/`.
- For OTA: install **Bonjour** (see main repo README) so the device appears in Tools → Port, or use another method.

This folder is kept so that when PlatformIO adds Arduino support for ESP32-C6, you can open it and use OTA by IP without changes.

## Requirements (when support exists)

- [PlatformIO](https://platformio.org/) (IDE or CLI)
- Board: ESP32-C6 (e.g. ESP32-C6-Touch-LCD-1.47)
- Config: edit `include/config.h` (WiFi, MQTT, OTA, pins)

## Build and upload (USB)

```bash
cd environmental_monitor_gui_pio
pio run
pio run -t upload
```

Select the correct serial port in PlatformIO or set `upload_port` in `platformio.ini`.

## OTA upload (by IP)

1. **First flash** the firmware once via USB (see above).
2. **Get the device IP**: open Serial Monitor (115200) and look for `OTA ready at 192.168.x.x`.
3. **Upload over WiFi**:
   - Edit `platformio.ini`: in the `[env:ota]` section set `upload_port = 192.168.x.x` (your device IP), or
   - Pass IP at upload time:
   ```bash
   pio run -e ota -t upload --upload-port 192.168.1.100
   ```

No mDNS/Bonjour needed: you only need the device IP.

## Project structure

- `platformio.ini` — board, framework, libs, envs (default + `ota`)
- `src/main.cpp` — application code (from Arduino sketch)
- `include/config.h` — WiFi, MQTT, OTA, TFT and I2C pins

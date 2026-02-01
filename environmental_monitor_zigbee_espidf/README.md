# Environmental Monitor — ESP-IDF GUI + Zigbee (ESP32-C6)

Project skeleton for porting the Arduino GUI (SCD41 + BME680 + TFT) to **ESP-IDF**, then adding **Zigbee** for Home Assistant. Zigbee on ESP32-C6 is supported only with ESP-IDF, not Arduino.

## Project layout

- `main/main.c` — app entry, I2C/SPI/GPIO init placeholders, stub `sensors_init` / `display_init` / `read_sensors` / `update_display`.
- `main/config.h` — pins and timing (same as Arduino `environmental_monitor_gui/config.h`).
- Port sensor/display logic from `../environmental_monitor_gui/`, then add Zigbee (esp-zigbee-sdk).

## Build (ESP-IDF v5.x)

**Environment:** `C:\Espressif\frameworks\esp-idf-v5.5.2` (tools in `C:\Espressif`).

1. Open **ESP-IDF 5.5 PowerShell** from the Start menu (so `idf.py` is on PATH), or in a normal PowerShell run:
   ```powershell
   $env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.5.2"
   $env:IDF_TOOLS_PATH = "C:\Espressif"
   . C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1
   ```

2. **Set target and build**
   ```powershell
   cd c:\Users\lucad\Cursor\environmental-monitor\environmental_monitor_zigbee_espidf
   idf.py set-target esp32c6
   idf.py build
   ```

3. **Flash / monitor**
   ```powershell
   idf.py -p COMx flash monitor
   ```
   Replace `COMx` with your board port (e.g. `COM3`).

Serial log will show placeholder messages (I2C/SPI/GPIO init, sensors_init, display_init, read_sensors, update_display) and the main loop delay.

## Next steps

1. **Port GUI** — Implement SCD41/BME680 drivers (I2C) and TFT driver (SPI + GPIO CS/DC/RST), reuse init sequence from Arduino `lcd_reg_init()` and layout from `drawScreen` / `drawCountdown`.
2. **Add Zigbee** — [ESP Zigbee SDK](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html); start from a ZED example, then report T/RH/CO2/IAQ via ZCL (e.g. 0x0402, 0x0405, manufacturer-specific).
3. **Pair in Home Assistant** — Coordinator in pairing mode; power device to join.

## References

- [ESP Zigbee SDK — ESP32-C6](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html)
- [Zigbee HA integration (ZHA)](https://www.home-assistant.io/integrations/zha/)


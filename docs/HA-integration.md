# Home Assistant Integration (Zigbee, ESP-IDF)

Zigbee on ESP32-C6 is supported **only with ESP-IDF**, not with the Arduino framework. To send environmental monitor data to Home Assistant over your existing Zigbee network, port the project to ESP-IDF and use the ESP Zigbee SDK.

## What you need

- **ESP-IDF** (Espressif IoT Development Framework), target `esp32c6`
- **ESP Zigbee SDK** (ZBOSS stack for ESP32-C6)
- Port the sensor logic (SCD41, BME680) and optionally the display to an ESP-IDF project
- ESP32-C6 joins your existing Zigbee network (ZHA / zigbee2mqtt coordinator) as an **End Device**
- Report temperature, humidity, and optionally CO2/IAQ via Zigbee clusters (e.g. ZCL Temperature 0x0402, Humidity 0x0405, or custom)

## Steps (outline)

1. Install ESP-IDF and set target `esp32c6`.
2. Clone and use [esp-zigbee-sdk](https://github.com/espressif/esp-zigbee-sdk); follow [ESP32-C6 Zigbee docs](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html).
3. Start from an end-device example (e.g. temperature/humidity sensor); add I2C drivers for SCD41 and BME680.
4. Implement ZCL reporting (Temperature, Humidity; for CO2/IAQ use manufacturer-specific or custom cluster if needed).
5. In Home Assistant: add device via ZHA (or your Zigbee stack); entities will appear from the reported attributes.

## Reference

- [ESP Zigbee SDK — ESP32-C6](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html)
- [environmental_monitor_zigbee_espidf/](../environmental_monitor_zigbee_espidf/) — README and outline for the ESP-IDF Zigbee project

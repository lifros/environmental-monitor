# Home Assistant Integration

## Option A: WiFi + MQTT (Arduino — environmental_monitor_gui)

The **environmental_monitor_gui** sketch (Arduino, ESP32-C6) can send sensor data to Home Assistant over WiFi via MQTT. No Zigbee hardware required.

### Requirements

- **Libraries**: Install **PubSubClient** (Nick O'Leary) or **PubSubClient3** (Holger Mueller) via Arduino Library Manager; both use `#include <PubSubClient.h>` and work with this sketch.
- **Broker**: MQTT broker reachable from the ESP (e.g. Mosquitto on the same LAN, or Home Assistant MQTT add-on).
- **Config**: In `environmental_monitor_gui/config.h` set `ENABLE_MQTT 1`, then:
  - `WIFI_SSID`, `WIFI_PASSWORD`
  - `MQTT_BROKER` (IP or hostname), `MQTT_PORT` (default 1883)
  - Optionally `MQTT_USER`, `MQTT_PASSWORD` if the broker uses auth.
  - `MQTT_DEVICE_ID` (e.g. `envmon_gui`) — used as topic prefix and for HA entity IDs.

### Behaviour

- On each measurement cycle the device publishes one JSON message to `<MQTT_DEVICE_ID>/state` with: `co2`, `temperature_scd41`, `humidity_scd41`, `temperature_bme`, `humidity_bme`, `pressure`, `iaq`.
- On first MQTT connect it publishes **Home Assistant MQTT discovery** configs so entities appear automatically under **Settings → Devices & services → MQTT**.
- To disable MQTT, set `ENABLE_MQTT 0` in `config.h`.

### Home Assistant

- Add the **MQTT** integration (if not already) and point it to your broker.
- After the ESP connects and publishes discovery, the sensors (e.g. `sensor.envmon_gui_co2`) appear and can be used in dashboards and automations.

---

## Option B: Zigbee (ESP-IDF)

Zigbee on ESP32-C6 is supported **only with ESP-IDF**, not with the Arduino framework. To send environmental monitor data to Home Assistant over your existing Zigbee network, port the project to ESP-IDF and use the ESP Zigbee SDK.

### What you need

- **ESP-IDF** (Espressif IoT Development Framework), target `esp32c6`
- **ESP Zigbee SDK** (ZBOSS stack for ESP32-C6)
- Port the sensor logic (SCD41, BME680) and optionally the display to an ESP-IDF project
- ESP32-C6 joins your existing Zigbee network (ZHA / zigbee2mqtt coordinator) as an **End Device**
- Report temperature, humidity, and optionally CO2/IAQ via Zigbee clusters (e.g. ZCL Temperature 0x0402, Humidity 0x0405, or custom)

### Steps (outline)

1. Install ESP-IDF and set target `esp32c6`.
2. Clone and use [esp-zigbee-sdk](https://github.com/espressif/esp-zigbee-sdk); follow [ESP32-C6 Zigbee docs](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html).
3. Start from an end-device example (e.g. temperature/humidity sensor); add I2C drivers for SCD41 and BME680.
4. Implement ZCL reporting (Temperature, Humidity; for CO2/IAQ use manufacturer-specific or custom cluster if needed).
5. In Home Assistant: add device via ZHA (or your Zigbee stack); entities will appear from the reported attributes.

### Reference

- [ESP Zigbee SDK — ESP32-C6](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/introduction.html)
- [environmental_monitor_zigbee_espidf/](../environmental_monitor_zigbee_espidf/) — README and outline for the ESP-IDF Zigbee project

# Home Assistant Integration

## WiFi + MQTT (Arduino — environmental_monitor_gui)

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

- On each measurement cycle the device publishes one JSON message to `<MQTT_DEVICE_ID>/state` with: `co2`, `temperature_scd41`, `humidity_scd41`.
- On first MQTT connect it publishes **Home Assistant MQTT discovery** configs so entities appear automatically under **Settings → Devices & services → MQTT**.
- To disable MQTT, set `ENABLE_MQTT 0` in `config.h`.

### Home Assistant

- Add the **MQTT** integration (if not already) and point it to your broker.
- After the ESP connects and publishes discovery, the sensors (e.g. `sensor.envmon_gui_co2`) appear and can be used in dashboards and automations.


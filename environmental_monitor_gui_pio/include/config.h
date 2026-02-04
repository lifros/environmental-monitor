/**
 * Configuration for ESP32-C6-Touch-LCD-1.47
 * I2C: expansion header (SDA=18, SCL=19).
 * TFT: pins from board schematic.
 */
#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

/** CO2 monitor: SCD41 only. T/RH from SCD41 for context. See docs/SCD41-specs-and-compensation.md. */

/** SCD41: chip temperature offset (0–20 °C per datasheet). Chip recalculates RH from corrected T. */
#define SCD41_TEMP_OFFSET_C (3.0f)

/** SCD41: sensor altitude in metres (0 = do not set, use default 0 m). Set for pressure compensation; then optionally persist. */
#define SCD41_SENSOR_ALTITUDE_M (0u)

/** SCD41: 1 = persist offset/altitude to EEPROM after setup (max 2000 writes lifetime). 0 = apply from config each boot. */
#define SCD41_PERSIST_SETTINGS (0)

/** SCD41: 1 = low power periodic (~30 s interval, ~3.2 mA); 0 = high performance (~5 s interval, ~15–18 mA). Use 30 s measure interval when 1. */
#define SCD41_LOW_POWER_PERIODIC (0)

#define MEASURE_INTERVAL_SEC 60

#define TFT_CS   14
#define TFT_DC   15
#define TFT_RST  22
#define TFT_MOSI 2
#define TFT_SCK  1
#define TFT_BL   23

#define TFT_W 172
#define TFT_H 320
#define TFT_COL_OFFSET 34
#define TFT_ROW_OFFSET 0
#define TFT_ROTATION 3

/* WiFi + MQTT (Home Assistant). Set ENABLE_MQTT 0 to disable. */
#define ENABLE_MQTT 1
#define WIFI_SSID     "home_iot"
#define WIFI_PASSWORD "HomeIot2024!"
#define MQTT_BROKER   "homeassistant.home.encryptio.it"
#define MQTT_PORT     1883
#define MQTT_USER     "mqtt_user"
#define MQTT_PASSWORD "30Ott2008!"
#define MQTT_DEVICE_ID "envmon_gui"

/* OTA (Over-The-Air) update. Requires WiFi (ENABLE_MQTT 1). Set ENABLE_OTA 0 to disable. */
#define ENABLE_OTA 1
#define OTA_HOSTNAME "envmon_gui"
#define OTA_PASSWORD ""

#endif

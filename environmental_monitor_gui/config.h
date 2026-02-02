/**
 * Configuration for ESP32-C6-Touch-LCD-1.47
 * I2C: expansion header (SDA=18, SCL=19).
 * TFT: pins from board schematic.
 */
#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

/** SCD41 self-heating: sensor reads this much above ambient; chip subtracts it (recalculates RH).
 *  Calibrated vs Aqara THP (ref T 19.6°C): SCD41 was 21.4°C → offset 4.3°C. */
#define SCD41_TEMP_OFFSET_C (4.3f)

/** BME680 software calibration vs Aqara THP (ref T 19.6°C, H 62.74%). Add to raw reading. */
#define BME680_TEMP_OFFSET_C   (-0.9f)   /* BME was 20.5°C */
#define BME680_HUMIDITY_OFFSET  (6.0f)   /* BME was 56.7% */

/** BME680 IAQ: gas resistance (ohm). R < R_MIN = worst, R > R_MAX = best. Tune after 10–30 min burn-in. */
#define IAQ_R_MIN 5000
#define IAQ_R_MAX 60000
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

/* WiFi + MQTT (Home Assistant). Set ENABLE_MQTT 0 to disable.
 * Library: PubSubClient (Nick O'Leary) or PubSubClient3 (Holger Mueller); both use PubSubClient.h */
#define ENABLE_MQTT 1
#define WIFI_SSID     "home_iot"
#define WIFI_PASSWORD "HomeIot2024!"
#define MQTT_BROKER   "homeassistant.home.encryptio.it"   /* or your HA / broker hostname */
#define MQTT_PORT     1883
#define MQTT_USER     "mqtt_user"                /* leave "" if no auth */
#define MQTT_PASSWORD "30Ott2008!"
#define MQTT_DEVICE_ID "envmon_gui"     /* used in topic and HA entity_id prefix */

#endif

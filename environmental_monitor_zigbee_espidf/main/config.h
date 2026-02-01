/**
 * Pin and config for ESP32-C6-Touch-LCD-1.47 (same as Arduino config.h).
 * I2C: expansion header. TFT: board schematic.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* I2C — SCD41, BME680 */
#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19
#define I2C_MASTER_FREQ_HZ 100000

/* TFT (SPI) */
#define TFT_CS_GPIO   14
#define TFT_DC_GPIO   15
#define TFT_RST_GPIO  22
#define TFT_MOSI_GPIO 2
#define TFT_SCK_GPIO  1
#define TFT_BL_GPIO   23

#define TFT_W 172
#define TFT_H 320
#define TFT_ROTATION 3

/* Timing */
#define MEASURE_INTERVAL_SEC 60

#endif

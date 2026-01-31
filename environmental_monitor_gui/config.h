/**
 * Configuration for ESP32-C6-Touch-LCD-1.47
 * I2C: expansion header (SDA=18, SCL=19).
 * TFT: pins from board schematic.
 */
#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

#define IAQ_R_MIN 5000
#define IAQ_R_MAX 60000

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

#endif

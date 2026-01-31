/**
 * Configuration for ESP32-C6-Touch-LCD-1.47
 * I2C: expansion header (SDA=18, SCL=19).
 * Display: verify pins from board schematic (SPI TFT 172×320).
 */
#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

/** BME680 IAQ: gas resistance range (ohm). Lower R = more VOCs = worse IAQ. */
#define IAQ_R_MIN 5000
#define IAQ_R_MAX 60000

/** TFT display (SPI). Check schematic for your board; these are placeholders. */
#define TFT_CS   7
#define TFT_DC   8
#define TFT_RST  9
#define TFT_MOSI 10
#define TFT_SCK  11

#define TFT_W 172
#define TFT_H 320

#endif

/**
 * Board and sensor configuration — ESP32-C6-Touch-LCD-1.47
 * I²C: SDA/SCL on right header (positions 6 and 5). GPIO from Waveshare schematic.
 */
#ifndef CONFIG_H
#define CONFIG_H

// I²C for SCD41, BME680 (and onboard touch AXS5106L)
// From schematic: ESP_SDA → GPIO18, ESP_SCL → GPIO19 (header right side, SCL pos 5, SDA pos 6)
#define I2C_SDA_GPIO  18
#define I2C_SCL_GPIO  19

// I²C bus (Wire)
#define I2C_FREQ_HZ   100000

// SCD41 I²C address: 0x62
// BME680 I²C address: 0x76 or 0x77

// How often to check sensors and update report (ms). Home IAQ: 60 s is common (ESPHome, etc.).
#define READ_INTERVAL_MS  60000

// Display (172×320 SPI, from Waveshare schematic)
#define LCD_DC_GPIO   15
#define LCD_CS_GPIO   14
#define LCD_SCK_GPIO  2
#define LCD_MOSI_GPIO 3
#define LCD_RST_GPIO  22
#define LCD_BL_GPIO   23

#endif // CONFIG_H

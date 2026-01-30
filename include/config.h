/**
 * Board and sensor configuration — ESP32-C6-Touch-LCD-1.47
 * I²C: SDA/SCL on right header (positions 6 and 5).
 * Verify GPIO numbers from board schematic if sensors are not detected.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// I²C for SCD41, BME680 (and onboard touch AXS5106L)
// Right header: SCL = pos 5, SDA = pos 6 — typical assignment on this board
#define I2C_SDA_GPIO  8
#define I2C_SCL_GPIO  9

// I²C bus (Wire)
#define I2C_FREQ_HZ   100000

// SCD41 I²C address: 0x62
// BME680 I²C address: 0x76 or 0x77

#endif // CONFIG_H

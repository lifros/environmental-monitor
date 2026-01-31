/**
 * Pin and I2C configuration for ESP32-C6-Touch-LCD-1.47
 * (expansion header I2C from schematic: SDA=GPIO18, SCL=GPIO19)
 */
#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

/** BME680 IAQ: gas resistance range (ohm). Lower R = more VOCs = worse IAQ. */
#define IAQ_R_MIN 5000
#define IAQ_R_MAX 60000

#endif

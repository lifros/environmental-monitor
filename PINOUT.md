# ESP32-C6-Touch-LCD-1.47 — Pinout

## Right side (from top)

| # | Label | Function        |
|---|-------|-----------------|
| 1 | VBAT  | Battery voltage |
| 2 | GND   | Ground          |
| 3 | GND   | Ground          |
| 4 | 3V3   | 3.3 V supply    |
| 5 | SCL   | I²C clock       |
| 6 | SDA   | I²C data        |
| 7 | 13    | GPIO 13         |
| 8 | 12    | GPIO 12         |
| 9 | 9     | GPIO 9          |
|10 | 8     | GPIO 8          |
|11 | 7     | GPIO 7          |

## Left side (from top)

| # | Label | Function  |
|---|-------|-----------|
| 1 | VBUS  | 5 V (USB) |
| 2 | GND   | Ground    |
| 3 | TXD   | UART TX   |
| 4 | RXD   | UART RX   |
| 5 | RST   | Reset     |
| 6 | 1     | GPIO 1    |
| 7 | 2     | GPIO 2    |
| 8 | 3     | GPIO 3    |
| 9 | 4     | GPIO 4    |
|10 | 5     | GPIO 5    |
|11 | 6     | GPIO 6    |

## I²C (SCD41, BME680)

- **SDA**, **SCL** — right side pins 6 and 5  
- **3V3**, **GND** — right side pins 4 and 2 or 3  

### GPIO for SDA/SCL (no numbers on silkscreen)

The SDA/SCL pins on the header are not labeled with GPIO numbers. You can find them in two ways:

1. **Schematic**  
   Download the board schematic and trace the nets **ESP_SDA** and **ESP_SCL** to the ESP32-C6 pins:
   - [Waveshare schematic PDF](https://files.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47/ESP32-C6-Touch-LCD-1.47-Schematic.pdf)  
   From that schematic, the expansion header I²C pins are:
   - **SDA (header pos 6) = GPIO 18**
   - **SCL (header pos 5) = GPIO 19**

2. **I²C scanner**  
   If the schematic does not match your board revision, use the project’s I²C scanner sketch (`i2c_scanner/`). With SDA=18, SCL=19 the following addresses have been verified:
   - **0x63** — onboard touch
   - **0x6B** — onboard IMU / touch
   - **0x62** — SCD41 (external)
   - **0x77** — BME680 (external)
   The project’s `config.h` sets `I2C_SDA_GPIO` and `I2C_SCL_GPIO` accordingly.

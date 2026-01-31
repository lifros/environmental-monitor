# ESP32-C6 1.47inch Touch Display Development Board

- 172×320 Resolution
- 262K Display Color
- Supports Wi-Fi 6 / BLE 5
- 160MHz RISC-V Processor

#### Ref: https://www.waveshare.com/esp32-c6-touch-lcd-1.47.htm?sku=31201
#### Wiki: https://www.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47

# Features

ESP32-C6-Touch-LCD-1.47 uses ESP32-C6FH8 chip with 2.4GHz WiFi 6 and Bluetooth BLE 5 support, integrates 8MB Flash, Onboard 1.47inch IPS display can smoothly run GUI programs such as LVGL. It is suitable for the quick development of the HMI and other ESP32-C6 applications.

Equipped with a high-performance 32-bit RISC-V processor with clock speed up to 160 MHz, and a low-power 32-bit RISC-V processor with clock speed up to 20MHz
Supports 2.4GHz Wi-Fi 6 (802.11 ax/b/g/n) and Bluetooth 5 (LE), with onboard antenna
Built-in 320KB ROM, 512KB HP SRAM, and 16KB LP SRAM, integrates 8MB Flash
Onboard 1.47inch capacitive touch LCD display, 172×320 resolution, 262K color
Adapting multiple IO interfaces, supports full-speed USB standard
Onboard TF card slot for external TF card storage of pictures or files
Supports accurate control such as flexible clock and multiple power modes to realize low power consumption in different scenarios

## Display specifications

| Parameter         | Value        | Parameter       | Value        |
|-------------------|--------------|-----------------|--------------|
| Display size      | 1.47 inch    | Resolution      | 172 × 320    |
| Display driver    | JD9853       | Touch driver    | AXS5106L     |
| Display interface | 4-wire SPI   | Touch interface | I²C          |
| Panel             | IPS          | Touch type      | Capacitive   |

## Pinout
### Left side (from top) | Right side (from top)

| Label | Function          | | Label | Function         |
|-------|-------------------|-|-------|------------------|
| VBUS  | 5 V (USB)         | | VBAT  | VBAT             |
| GND   | GND               | | GND   | GND              |
| TXD   | GPIO16 (UART0_TX) | | GND   | GND              |
| RXD   | GPIO17 (UART0_RX) | | 3V3   | 3.3 V            |
| RST   | Reset             | | SCL   | GPIO19 (I²C_SCL) |
| 1     | GPIO1 (SPI_SCLK)  | | SDA   | GPIO18 (I²C_SDA) |
| 2     | GPIO2 (SPI_MOSI)  | | 13    | GPIO13 (USB_DP)  |
| 3     | GPIO3 (SPI_MISO)  | | 12    | GPIO12 (USB_DN)  |
| 4     | GPIO4             | | 9     | GPIO9            |
| 5     | GPIO5             | | 8     | GPIO8            |
| 6     | GPIO6             | | 7     | GPIO7            |

## Touch LCD

| Label  | Function |
|--------|----------|
| GPIO1  | LCD_CLK  |
| GPIO2  | LCD_DIN  |
| GPIO14 | LCD_CS   |
| GPIO15 | LCD_DC   |
| GPIO18 | TP_SDA   |
| GPIO19 | TP_SCL   |
| GPIO20 | TP_RST   |
| GPIO21 | TP_INT   |
| GPIO22 | LCD_RST  |
| GPIO23 | LCD_BL   |

## SD Card

| Label | Function |
|-------|----------|
| GPIO3 | SD_MISO  |
| GPIO2 | SD_MOSI  |
| GPIO1 | SD_CLK   |
| GPIO4 | SD_CS    |

## QMI8658

| Label  | Function |
|--------|----------|
| GPIO18 | IMU_SDA  |
| GPIO19 | IMU_SCL  |
| GPIO5  | IMU_INT1 |
| GPIO6  | IMU_INT2 |

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

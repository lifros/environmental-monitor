/**
 * Environmental monitor — ESP-IDF skeleton for GUI + Zigbee (ESP32-C6).
 * Placeholders: I2C (SCD41, BME680), SPI (TFT), GPIO (backlight).
 * Port sensor/display logic from environmental_monitor_gui, then add Zigbee.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "env_mon";

#define I2C_MASTER_NUM   I2C_NUM_0
#define SPI_HOST_ID      SPI2_HOST

/* Placeholder (TFT device added when porting display driver). */
static spi_device_handle_t spi_tft_handle = NULL;

/* Legacy I2C API for maximum compatibility; can switch to i2c_master.h later. */
static esp_err_t i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) return ret;
    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "I2C init OK (SDA=%d SCL=%d)", I2C_SDA_GPIO, I2C_SCL_GPIO);
    return ESP_OK;
}

static esp_err_t spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TFT_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = TFT_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    /* Add TFT device later when porting display driver (CS, DC, RST). */
    ESP_LOGI(TAG, "SPI init OK (MOSI=%d SCK=%d)", TFT_MOSI_GPIO, TFT_SCK_GPIO);
    return ESP_OK;
}

static void gpio_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TFT_BL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(TFT_BL_GPIO, 1);
    ESP_LOGI(TAG, "GPIO backlight ON (pin %d)", TFT_BL_GPIO);
}

/* Placeholder: init SCD41 and BME680 over I2C. */
static void sensors_init(void)
{
    ESP_LOGI(TAG, "sensors_init (placeholder — port SCD41/BME680 drivers)");
}

/* Placeholder: init TFT and run board init sequence. */
static void display_init(void)
{
    (void)spi_tft_handle;
    ESP_LOGI(TAG, "display_init (placeholder — port Arduino_GFX + lcd_reg_init)");
}

/* Placeholder: read CO2, T, RH, pressure, IAQ. */
static void read_sensors(void)
{
    ESP_LOGI(TAG, "read_sensors (placeholder)");
}

/* Placeholder: draw env data and countdown on TFT. */
static void update_display(void)
{
    ESP_LOGI(TAG, "update_display (placeholder)");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Environmental monitor — ESP-IDF (GUI + Zigbee)");

    esp_err_t ret = i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed, continuing without sensors");
    }

    ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI init failed, continuing without display");
    }

    gpio_init();
    sensors_init();
    display_init();

    while (1) {
        read_sensors();
        update_display();
        vTaskDelay(pdMS_TO_TICKS(MEASURE_INTERVAL_SEC * 1000));
    }
}

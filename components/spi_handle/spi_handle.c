#include "spi_handle.h"

void spi_init(void)
{
    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .sclk_io_num = SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize the SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("SPI", "SPI bus initialized successfully");
    }
}

void spi_add_device(uint8_t cs_num, spi_device_handle_t *spi_device)
{
    spi_device_interface_config_t devcfg = {
        // configure device_structure
        .clock_speed_hz = 12 * 1000 * 1000,                             // Clock out at 12 MHz
        .mode = 0,                                                      // SPI mode 0: CPOL:-0 and CPHA:-0
        .spics_io_num = cs_num,                                     // This field is used to specify the GPIO pin that is to be used as CS'
        .queue_size = 7,                                                // We want to be able to queue 7 transactions at a time
    };

    esp_err_t ret = spi_bus_add_device(ESP_SPI_HOST, &devcfg, spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to add SPI device: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("SPI", "SPI device added successfully");
    }


}
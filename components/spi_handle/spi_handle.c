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

#include "sd.h"

static const char *TAG = "sd_spi_write";

esp_err_t sd_init(void){
    esp_err_t ret;

    // Pull CS high to avoid spurious signals during initialization
    gpio_reset_pin(SD_CS);
    gpio_set_direction(SD_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(SD_CS, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Short delay to ensure CS is stable before SPI init
    
    // 1) Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Choose a host. On ESP32, HSPI_HOST (SPI2) or VSPI_HOST (SPI3) depending on IDF.
    // Many examples use SPI2_HOST / SPI3_HOST in newer IDF.
    spi_host_device_t host = SPI2_HOST;

    // Initialize the SPI bus
    ret = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed (%s)", esp_err_to_name(ret));
        return ret;
    }
    
    // 2) Configure SD device on SPI
    sdmmc_host_t host_cfg = SDSPI_HOST_DEFAULT();
    host_cfg.max_freq_khz = 1000;   // 4 MHz (try 1000 or even 400 if still failing)
    host_cfg.slot = host;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_CS;
    slot_cfg.host_id = host;

    // 3) Mount FAT filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,  // set true if you want auto-format on failure (careful!)
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host_cfg, &slot_cfg, &mount_cfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card (%s)", esp_err_to_name(ret));
        spi_bus_free(host);
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

esp_err_t sd_write(void){
    const char *path = MOUNT_POINT "/hello.txt";
    FILE *f = fopen(path, "a");   // "w" overwrite, "a" append
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    } else {
        fprintf(f, "Hello from ESP-IDF over SPI!\n");
        fclose(f);
        ESP_LOGI(TAG, "Wrote to %s", path);
    }
    return ESP_OK;
}
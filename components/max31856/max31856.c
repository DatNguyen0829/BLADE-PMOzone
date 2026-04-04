#include "max31856.h"

const char *SPI_TAG = "MAX31856";

esp_err_t max31856_init(spi_device_handle_t *spi_device) {

    // Pull CS high before SD init
    gpio_reset_pin(MAX_CS);
    gpio_set_direction(MAX_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(MAX_CS, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configure MAX31856 as SPI device interface
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000, // 1 MHz
        .mode = 1, // SPI mode 1
        .spics_io_num = MAX_CS, // CS pin
        .queue_size = 7,
    };

    if (spi_bus_add_device(SPI2_HOST, &dev_cfg, spi_device) != ESP_OK) {
        ESP_LOGE("MAX31856", "Failed to add SPI device");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t max31856_write_register(spi_device_handle_t *spi_device, uint8_t reg_addr, uint8_t data){
    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data = { (uint8_t)(reg_addr | 0x80), data },
        .length = 16,
    };

    esp_err_t ret = spi_device_polling_transmit(*spi_device, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SPI_TAG, "SPI write failed for reg 0x%02X", reg_addr);
        return ESP_FAIL;
    }

    ESP_LOGI(SPI_TAG, "Wrote 0x%02X to reg 0x%02X", data, reg_addr);
    return ESP_OK;
}

uint8_t max31856_read_register(spi_device_handle_t *spi_device, uint8_t reg){
    spi_transaction_t trans_desc = {0};
    uint8_t tx_data[2] = { (uint8_t)(reg & 0x7F), 0x00 };
    uint8_t rx_data[2] = {0};


    trans_desc.length = 16;
    trans_desc.tx_buffer = tx_data;
    trans_desc.rx_buffer = rx_data;

    esp_err_t ret = spi_device_polling_transmit(*spi_device, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SPI_TAG, "SPI read failed for reg 0x%02X", reg);
        return 0;
    }

    ESP_LOGI(SPI_TAG, "Read reg 0x%02X = 0x%02X", reg, rx_data[1]);
    return rx_data[1];
}

esp_err_t max31856_configure(spi_device_handle_t *spi_device){
    
    uint8_t cr0 = 0;
    uint8_t cr1 = 0;
    uint8_t mask = 0x00;

    /* CR0:
       - one-shot mode
       - clear existing faults
       - enable open-circuit fault detection
    */
    cr0 = 0x02 | 0x20;

    /* CR1:
       - set thermocouple type to K
    */
    cr1 = MAX_K_TYPE;

    if(max31856_write_register(spi_device, MAX_CRO, cr0) != ESP_OK)
    {
        ESP_LOGE(SPI_TAG, "Failed to write CR0");
        return ESP_FAIL;
    }
    if(max31856_write_register(spi_device, MAX_CR1, cr1) != ESP_OK)
    {
        ESP_LOGE(SPI_TAG, "Failed to write CR1");
        return ESP_FAIL;
    }
    if(max31856_write_register(spi_device, MAX_FAULT, mask) != ESP_OK)
    {
        ESP_LOGE(SPI_TAG, "Failed to write FAULT");
        return ESP_FAIL;
    }

    ESP_LOGI(SPI_TAG, "MAX31856 configured");

    // ESP_LOGI(SPI_TAG, "CR0: 0x%02X", max31856_read_register(spi_device, MAX_CRO));
    // ESP_LOGI(SPI_TAG, "CR1: 0x%02X", max31856_read_register(spi_device, MAX_CR1));
    // ESP_LOGI(SPI_TAG, "FAULT: 0x%02X", max31856_read_register(spi_device, MAX_FAULT));
    // ESP_LOGI(SPI_TAG, "SR: 0x%02X", max31856_read_register(spi_device, MAX_SR));

    return ESP_OK;
}

uint8_t max31856_read_fault(spi_device_handle_t *spi_device)
{
    return max31856_read_register(spi_device, MAX_SR);
}


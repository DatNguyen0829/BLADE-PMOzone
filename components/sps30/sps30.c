#include "sps30.h"

const static char *TAG = "SPS30";

/* ---- SPS30 commands and parsing ----
    Following functions implement the SPS30 sensor communication protocol */

uint8_t SPS30_CalcCrc(const uint8_t data[2])
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

esp_err_t sps30_start(i2c_master_dev_handle_t dev_handle)
{
    uint8_t crc = SPS30_CalcCrc((uint8_t[]){0x05, 0x00});   // integer format
    uint8_t cmd[] = {0x00, 0x10, 0x05, 0x00, crc};    // pointer 0x0010 + payload
    return i2c_master_transmit(dev_handle, cmd, sizeof(cmd), -1);
}

esp_err_t sps30_ready(i2c_master_dev_handle_t dev_handle, bool *ready)
{
    uint8_t cmd[] = {0x02, 0x02};
    uint8_t rx[3] = {0};

    esp_err_t err = i2c_master_transmit(dev_handle, cmd, sizeof(cmd), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pointer write failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    err = i2c_master_receive(dev_handle, rx, sizeof(rx), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "ready raw = %02X %02X %02X", rx[0], rx[1], rx[2]);

    uint8_t crc = SPS30_CalcCrc((uint8_t[]){rx[0], rx[1]});
    if (crc != rx[2]) {
        ESP_LOGE(TAG, "Ready CRC failed: got 0x%02X expected 0x%02X, data=[0x%02X 0x%02X]",
                 rx[2], crc, rx[0], rx[1]);
        return ESP_ERR_INVALID_CRC;
    }

    *ready = (rx[1] == 0x01);
    return ESP_OK;
}

esp_err_t sps30_read_pm25(i2c_master_dev_handle_t dev_handle, uint16_t *pm25)
{
    uint8_t cmd[] = {0x03, 0x00};
    uint8_t rx[30] = {0};

    esp_err_t err = i2c_master_transmit(dev_handle, cmd, sizeof(cmd), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PM pointer write failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    err = i2c_master_receive(dev_handle, rx, sizeof(rx), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PM read failed: %s", esp_err_to_name(err));
        return err;
    }

    // Check all CRC triplets
    for (int i = 0; i < 30; i += 3) {
        uint8_t crc = SPS30_CalcCrc((uint8_t[]){rx[i], rx[i + 1]});
        if (crc != rx[i + 2]) {
            ESP_LOGE(TAG,
                     "Measurement CRC failed at triplet %d: got 0x%02X expected 0x%02X data=[0x%02X 0x%02X]",
                     i / 3, rx[i + 2], crc, rx[i], rx[i + 1]);
            return ESP_ERR_INVALID_CRC;
        }
    }

    // Integer output mode:
    // PM1.0  = bytes 0,1,2
    // PM2.5  = bytes 3,4,5
    *pm25 = ((uint16_t)rx[3] << 8) | rx[4];

    ESP_LOGI(TAG, "PM raw bytes: %02X %02X %02X %02X %02X %02X",
             rx[0], rx[1], rx[2], rx[3], rx[4], rx[5]);

    return ESP_OK;
}

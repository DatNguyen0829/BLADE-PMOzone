#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define SPS30_I2C_ADDR 0x69

/* ---- SPS30 commands and parsing ----
    Following functions implement the SPS30 sensor communication protocol */

uint8_t SPS30_CalcCrc(const uint8_t data[2]);
    
esp_err_t sps30_start(i2c_master_dev_handle_t dev_handle);

esp_err_t sps30_ready(i2c_master_dev_handle_t dev_handle, bool *ready);

esp_err_t sps30_read_pm25(i2c_master_dev_handle_t dev_handle, uint16_t *pm25);

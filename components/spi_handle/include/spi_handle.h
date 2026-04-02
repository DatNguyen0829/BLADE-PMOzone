#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_CLK       18
#define ESP_SPI_HOST  SPI2_HOST

void spi_init(void);

void spi_add_device(uint8_t cs_num, spi_device_handle_t *spi_device);
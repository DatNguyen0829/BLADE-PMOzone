#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define MS5611_I2C_ADDR 0x77   
#define MS5611_D1_OSR_4096 0x48
#define MS5611_D2_OSR_4096 0x58

void ms5611_reset(i2c_master_dev_handle_t dev_handle);
void ms5611_read_prom(i2c_master_dev_handle_t dev_handle);
void ms5611_read_conversion(i2c_master_dev_handle_t dev_handle, uint8_t cmd);
int32_t ms5611_calculateTemperature();
int32_t ms5611_calculatePressure();

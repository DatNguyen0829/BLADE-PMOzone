#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

/* --- I2C handle for I2C Devices --- */ 

#define I2C_MASTER_SCL_IO      32
#define I2C_MASTER_SDA_IO      33
#define I2C_MASTER_FREQ_HZ     100000
#define I2C_MASTER_NUM         I2C_NUM_0

void i2c_master_init(i2c_master_bus_handle_t *bus_handle);

void i2c_add_device(uint8_t address, i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);


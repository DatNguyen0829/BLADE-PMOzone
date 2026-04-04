#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define MAX_CS     5

// Register addresses
#define MAX_CRO               0x00
#define MAX_CR1               0x01
#define MAX_FAULT             0x02  // Fault Mask register
#define MAX_SR                0x0F  // Fault status register

// -- Thermocouple Type --
#define MAX_K_TYPE            0x03

// --- These 3 will be used to read thermocouple temperature ---
#define MAX31856_LTCBH_REG    0x0C
#define MAX31856_LTCBM_REG    0x0D
#define MAX31856_LTCBL_REG    0x0E

// Initalizes MAX31856 on the SPI bus
esp_err_t max31856_init(spi_device_handle_t *spi_device);

// Read and Write to a MAX31856 register
esp_err_t max31856_write_register(spi_device_handle_t *spi_device, uint8_t reg_addr, uint8_t data);

uint8_t max31856_read_register(spi_device_handle_t *spi_device, uint8_t reg);

// Configure MAX31856 
esp_err_t max31856_configure(spi_device_handle_t *spi_device);

// Read fault status
uint8_t max31856_read_fault(spi_device_handle_t *spi_device);

// read junction temperature from MAX31856
float max31856_read_thermocouple_temp(spi_device_handle_t *spi_device);

 
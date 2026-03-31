#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"


#define ZE27O3_UART_NUM UART_NUM_2
#define ZE27O3_UART_TX_PIN 17
#define ZE27O3_UART_RX_PIN 16
#define ZE27O3_UART_BAUD_RATE 9600
#define ZE27O3_UART_BUF_SIZE 256

/* ---- ZE27O3 commands and parsing ----
    Following functions implement the ZE27O3 sensor communication protocol */

esp_err_t ze27_uart_init(void);
uint8_t ze27_checksum(const uint8_t *buf, size_t len);
esp_err_t ze27o3_readActiveUpload(uint16_t *o3_ppb);
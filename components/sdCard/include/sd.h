#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "esp_err.h"
#include "spi_handle.h"

#define SD_CS        27
#define MOUNT_POINT "/sdcard"

esp_err_t sd_init(void);
esp_err_t sd_write(void);
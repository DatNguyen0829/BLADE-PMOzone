#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"

#include "i2c_handle.h"
#include "spi_handle.h"
#include "sps30.h"
#include "ms5611.h"
#include "ze27o3.h"
#include "sd.h"
#include "max31856.h"

/* -------------------- Handles -------------------- */
i2c_master_bus_handle_t i2c_bus_handle;
i2c_master_dev_handle_t sps30_dev_handle;
i2c_master_dev_handle_t ms5611_dev_handle;
spi_device_handle_t max31856_spi_handle;
static SemaphoreHandle_t spi_mutex; // SPI_MUTEX

static QueueHandle_t telemetry_queue;

static const char *TAG = "APP";

/* -------------------- Telemetry Types -------------------- */
typedef enum {
    TELEMETRY_SRC_I2C,
    TELEMETRY_SRC_MAX31856,
    TELEMETRY_SRC_ZE27O3
} telemetry_source_t;

typedef struct {
    telemetry_source_t source;
    union {
        struct {
            uint16_t sps30_pm25;
            int32_t ms5611_temperature; 
            int32_t ms5611_pressure;    
            bool sps30_valid;
            bool ms5611_valid;
        } i2c;

        struct {
            float thermocouple_temp;
            bool valid;
        } max31856;

        struct {
            uint16_t o3_ppb;
            bool valid;
        } ze27o3;
    } data;
} telemetry_msg_t;

/* Latest full snapshot used by telemetry_task */
typedef struct {
    uint16_t sps30_pm25;
    int32_t ms5611_temperature;
    int32_t ms5611_pressure;
    float max31856_temp;
    uint16_t ze27o3_o3_ppb;

    bool sps30_valid;
    bool ms5611_valid;
    bool max31856_valid;
    bool ze27o3_valid;
} telemetry_snapshot_t;

/* -------------------- Task Prototypes -------------------- */
static void i2c_task(void *arg);
static void max31856_task(void *arg);
static void ze27o3_task(void *arg);
static void telemetry_task(void *arg);

void app_main(void)
{
    telemetry_queue = xQueueCreate(16, sizeof(telemetry_msg_t));
    if (telemetry_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create telemetry queue");
        return;
    }

    /* ---------- I2C Init ---------- */
    i2c_master_init(&i2c_bus_handle);
    ESP_LOGI(TAG, "I2C bus initialized");

    // i2c_add_device(SPS30_I2C_ADDR, &i2c_bus_handle, &sps30_dev_handle);
    // ESP_LOGI(TAG, "SPS30 added");

    i2c_add_device(MS5611_I2C_ADDR, &i2c_bus_handle, &ms5611_dev_handle);
    ESP_LOGI(TAG, "MS5611 added");

    // ESP_ERROR_CHECK(sps30_start(sps30_dev_handle));
    // ESP_LOGI(TAG, "SPS30 measurement started");
    vTaskDelay(pdMS_TO_TICKS(50));

    ms5611_reset(ms5611_dev_handle);
    ESP_LOGI(TAG, "MS5611 reset complete");
    vTaskDelay(pdMS_TO_TICKS(10));
    ms5611_read_prom(ms5611_dev_handle);

    /* ---------- ZE27O3 UART Init ---------- */
    // ESP_ERROR_CHECK(ze27_uart_init());
    // ESP_LOGI(TAG, "ZE27O3 UART initialized");

    /* ---------- SPI / SD / MAX31856 Init ---------- */
    spi_init();
    ESP_LOGI(TAG, "SPI bus initialized");

    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex");
        return;
    }

    if (sd_init() == ESP_OK) {
        ESP_LOGI(TAG, "SD card initialized");
    } else {
        ESP_LOGE(TAG, "SD card initialization failed");
    }

    if (max31856_init(&max31856_spi_handle) == ESP_OK) {
        ESP_LOGI(TAG, "MAX31856 initialized");
    } else {
        ESP_LOGE(TAG, "MAX31856 init failed");
    }

    if (max31856_configure(&max31856_spi_handle) == ESP_OK) {
        ESP_LOGI(TAG, "MAX31856 configured");
    } else {
        ESP_LOGE(TAG, "MAX31856 config failed");
    }

    /* ---------- Create Tasks ---------- */
    xTaskCreate(i2c_task, "i2c_task", 4096, NULL, 5, NULL);
    xTaskCreate(max31856_task, "max31856_task", 4096, NULL, 5, NULL);
    // xTaskCreate(ze27o3_task, "ze27o3_task", 4096, NULL, 5, NULL);
    xTaskCreate(telemetry_task, "telemetry_task", 4096, NULL, 10, NULL);
}

/* -------------------- I2C Task -------------------- */
static void i2c_task(void *arg)
{
    telemetry_msg_t msg;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        msg.source = TELEMETRY_SRC_I2C;

        // /* ---- SPS30 ---- */
        // bool sps30_ready_flag = false;
        // esp_err_t err = sps30_ready(sps30_dev_handle, &sps30_ready_flag);
        // if (err == ESP_OK && sps30_ready_flag) {
        //     err = sps30_read_pm25(sps30_dev_handle, &msg.data.i2c.sps30_pm25);
        //     if (err == ESP_OK) {
        //         msg.data.i2c.sps30_valid = true;
        //     } else {
        //         ESP_LOGW(TAG, "Failed to read SPS30 PM2.5: %s", esp_err_to_name(err));
        //     }
        // }

        /* ---- MS5611 ---- */
        ms5611_read_conversion(ms5611_dev_handle, MS5611_D1_OSR_4096);
        ms5611_read_conversion(ms5611_dev_handle, MS5611_D2_OSR_4096);
        msg.data.i2c.ms5611_temperature = ms5611_calculateTemperature();
        msg.data.i2c.ms5611_pressure = ms5611_calculatePressure();
        msg.data.i2c.ms5611_valid = true;

        ESP_LOGI(TAG,
                     "MS5611 Temp: %.2f C, Pressure: %.2f",
                     msg.data.i2c.ms5611_temperature / 100.0,
                     msg.data.i2c.ms5611_pressure / 100.0);

        if (xQueueSend(telemetry_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Failed to send I2C telemetry");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* -------------------- MAX31856 Task -------------------- */
static void max31856_task(void *arg)
{
    telemetry_msg_t msg;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        msg.source = TELEMETRY_SRC_MAX31856;

        if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            float temp = max31856_read_thermocouple_temp(&max31856_spi_handle);
            xSemaphoreGive(spi_mutex);

            msg.data.max31856.thermocouple_temp = temp;
            msg.data.max31856.valid = true;

            ESP_LOGI(TAG, "MAX31856 Thermocouple Temp: %.2f C", temp);

            if (xQueueSend(telemetry_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Failed to send MAX31856 telemetry");
            }
        } else {
            ESP_LOGW(TAG, "SPI mutex timeout in max31856_task");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
/* -------------------- ZE27O3 Task -------------------- */
static void ze27o3_task(void *arg)
{
    telemetry_msg_t msg;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        msg.source = TELEMETRY_SRC_ZE27O3;

        esp_err_t err = ze27o3_readActiveUpload(&msg.data.ze27o3.o3_ppb);
        if (err == ESP_OK) {
            msg.data.ze27o3.valid = true;
            ESP_LOGI(TAG, "ZE27O3 O3: %u ppb", msg.data.ze27o3.o3_ppb);

            if (xQueueSend(telemetry_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Failed to send ZE27O3 telemetry");
            }
        } else {
            ESP_LOGW(TAG, "Failed to read ZE27O3: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* -------------------- Telemetry Task -------------------- */
static void telemetry_task(void *arg)
{
    telemetry_msg_t msg;
    telemetry_snapshot_t latest = {0};

    TickType_t last_log_time = xTaskGetTickCount();
    const TickType_t log_period = pdMS_TO_TICKS(250);

    while (1) {
        /* Wait up to 1 second for new data */
        if (xQueueReceive(telemetry_queue, &msg, pdMS_TO_TICKS(200)) == pdPASS) {
            switch (msg.source) {
                case TELEMETRY_SRC_I2C:
                    if (msg.data.i2c.sps30_valid) {
                        latest.sps30_pm25 = msg.data.i2c.sps30_pm25;
                        latest.sps30_valid = true;
                    }
                    if (msg.data.i2c.ms5611_valid) {
                        latest.ms5611_temperature = msg.data.i2c.ms5611_temperature;
                        latest.ms5611_pressure = msg.data.i2c.ms5611_pressure;
                        latest.ms5611_valid = true;
                    }
                    break;

                case TELEMETRY_SRC_MAX31856:
                    if (msg.data.max31856.valid) {
                        latest.max31856_temp = msg.data.max31856.thermocouple_temp;
                        latest.max31856_valid = true;
                    }
                    break;

                case TELEMETRY_SRC_ZE27O3:
                    if (msg.data.ze27o3.valid) {
                        latest.ze27o3_o3_ppb = msg.data.ze27o3.o3_ppb;
                        latest.ze27o3_valid = true;
                    }
                    break;

                default:
                    break;
            }
        }

        /* Log once per second using latest values */
        if ((xTaskGetTickCount() - last_log_time) >= log_period) {
            char telemetry_data[256];

            snprintf(
                telemetry_data,
                sizeof(telemetry_data),
                "PM2.5=%s%u, MS5611_Temp=%s%.2f, MS5611_Press=%s%.2f, MAX31856_Temp=%s%.2f, O3=%s%u\n",
                latest.sps30_valid ? "" : "NA,",
                latest.sps30_valid ? latest.sps30_pm25 : 0,

                latest.ms5611_valid ? "" : "NA,",
                latest.ms5611_valid ? (latest.ms5611_temperature / 100.0) : 0.0,

                latest.ms5611_valid ? "" : "NA,",
                latest.ms5611_valid ? (latest.ms5611_pressure / 100.0) : 0.0,

                latest.max31856_valid ? "" : "NA,",
                latest.max31856_valid ? latest.max31856_temp : 0.0,

                latest.ze27o3_valid ? "" : "NA,",
                latest.ze27o3_valid ? latest.ze27o3_o3_ppb : 0
            );

            if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                esp_err_t err = sd_write(telemetry_data);
                xSemaphoreGive(spi_mutex);

                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to write telemetry to SD card: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "Logged: %s", telemetry_data);
                }

            } else {
                ESP_LOGW(TAG, "SPI mutex timeout in telemetry_task");
            }
            
            last_log_time = xTaskGetTickCount();
        }
    }
}
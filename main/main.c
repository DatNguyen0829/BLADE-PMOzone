#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "i2c_handle.h"
#include "spi_handle.h"
#include "sps30.h"
#include "ms5611.h"
#include "ze27o3.h"
#include "sd.h"
#include "max31856.h"

/* --- I2C bus variables --- */
i2c_master_bus_handle_t i2c_bus_handle;
i2c_master_dev_handle_t sps30_dev_handle;
i2c_master_dev_handle_t ms5611_dev_handle;


/* --- SPI bus variables --- */
spi_device_handle_t max31856_spi_handle;

/* Data Structure for all measurements */
typedef struct {
    uint16_t SPS30_pm25;
    int32_t MS5611_temperature;
    int32_t MS5611_pressure;
    uint16_t ZE27O3_o3_ppb;
} Telemetry;
static Telemetry telemetry = {0};


// Prototype for the task function
static void i2c_task(void *arg);
static void ze27o3_task(void *arg);

void app_main(void)
{
    // Initialize I2C bus
    i2c_master_init(&i2c_bus_handle);
    printf("I2C bus initialized\n");

    // Add SPS30 device to the bus
    i2c_add_device(SPS30_I2C_ADDR, &i2c_bus_handle, &sps30_dev_handle);
    printf("SPS30 device added to I2C bus\n");

    // Add MS5611 device to the bus
    i2c_add_device(MS5611_I2C_ADDR, &i2c_bus_handle, &ms5611_dev_handle);
    printf("MS5611 device added to I2C bus\n");

    // Start SPS30 measurement
    ESP_ERROR_CHECK(sps30_start(sps30_dev_handle));
    printf("SPS30 measurement started\n");
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for measurement to start

    //Reset ms5611 sensor
    ms5611_reset(ms5611_dev_handle);
    printf("MS5611 sensor reset complete.\n");
    vTaskDelay(pdMS_TO_TICKS(10));           // Wait for reset to complete
    ms5611_read_prom(ms5611_dev_handle);     //Read device Prom values

    // Initialize ZE27O3 UART
    ESP_ERROR_CHECK(ze27_uart_init());

    // Create a task to read I2C data periodically
    xTaskCreate(i2c_task, "i2c_task", 2048, &telemetry, 5, NULL);

    // Create a task to read ZE27O3 data periodically
    xTaskCreate(ze27o3_task, "ze27o3_task", 2048, &telemetry, 5, NULL);

    // Initialize spi_bus
    spi_init();
    printf("SPI bus initialized\n");

    if(sd_init() == ESP_OK){
        printf("SD card initialized and mounted successfully\n");
    } else {
        printf("Failed to initialize SD card\n");
    }

    if (max31856_init(&max31856_spi_handle) == ESP_OK) {
        printf("MAX31856 initialized successfully\n");
    } else {
        printf("Failed to initialize MAX31856\n");
    }

    if (max31856_configure(&max31856_spi_handle) == ESP_OK) {
        printf("MAX31856 configured successfully\n");
    } else {
        printf("Failed to configure MAX31856\n");
    }

    while(1){
        if (sd_write() == ESP_OK){
            printf("Data written to SD card successfully\n");
        } else{
            printf("Failed to write data to SD card\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Write to SD card every 5 seconds
    }
}

static void i2c_task(void *arg){
    Telemetry *telemetry = (Telemetry *)arg;
    while(1){
        // ---- SPS30 ----
        bool SPS30_readyFlag = false;
        esp_err_t err = sps30_ready(sps30_dev_handle, &SPS30_readyFlag);
        if (err != ESP_OK) {
            printf("Failed to check SPS30 readiness: %s\n", esp_err_to_name(err));
        }

        if (SPS30_readyFlag) {
            err = sps30_read_pm25(sps30_dev_handle, &telemetry->SPS30_pm25);
            if (err != ESP_OK) {
                printf("Failed to read SPS30 PM2.5: %s\n", esp_err_to_name(err));
            } else {
                printf("SPS30 PM2.5: %d\n", telemetry->SPS30_pm25);
            }
        }else {
            printf("SPS30 not ready yet\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Read sensors every 1 second

        // ---- MS5611 ----
        //Read D1 conversion result
        ms5611_read_conversion(ms5611_dev_handle, MS5611_D1_OSR_4096);
        //Read D2 conversion result
        ms5611_read_conversion(ms5611_dev_handle, MS5611_D2_OSR_4096);
        //Calculates Temperature 
        telemetry->MS5611_temperature = ms5611_calculateTemperature();
        telemetry->MS5611_pressure = ms5611_calculatePressure();
        printf("MS5611 Temperature: %.2f, Pressure: %.2f\n", telemetry->MS5611_temperature/100.0, telemetry->MS5611_pressure/100.0);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Read sensors every 1 second
    }
}

static void ze27o3_task(void *arg){
    Telemetry *telemetry = (Telemetry *)arg;
    while(1){
        ESP_ERROR_CHECK(ze27o3_readActiveUpload(&telemetry->ZE27O3_o3_ppb));
    }

}
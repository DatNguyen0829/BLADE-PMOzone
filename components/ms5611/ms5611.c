#include "ms5611.h"

// Calculation variabless
uint16_t prom_data[7] = {0};
uint32_t D1;
uint32_t D2;
int32_t dT;
int64_t OFF;
int64_t OFF2;
int64_t SENS;
int64_t SENS2;
int32_t T2;


const static char *TAG = "MS5611";

void ms5611_reset(i2c_master_dev_handle_t dev_handle) {
    uint8_t rst = 0x1E;
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &rst, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(3)); // datasheet: ~2.8ms max
}

void ms5611_read_prom(i2c_master_dev_handle_t dev_handle) {

    for (int i = 0; i < 6; i++) {
        // Send PROM read command
        uint8_t cmd = 0xA2 + (i*2); // Send Prom Command
        ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, -1));
        // Read PROM data
        uint8_t data[2];
        ESP_ERROR_CHECK(i2c_master_receive(dev_handle, data, 2, -1));
        prom_data[i+1] = ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
     }
}

void ms5611_read_conversion(i2c_master_dev_handle_t dev_handle, uint8_t cmd) {
    // ESP_LOGI(TAG, "Sending conversion command: 0x%02X", cmd);
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for conversion to complete

    //ADC READ SEQUENCE
    uint8_t read_cmd = 0x00; // Command to read ADC result
    uint8_t data[3];
    // Send ADC read command
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &read_cmd, 1, -1));

    // Read 3 bytes of either D1 or D2 result
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, data, 3, -1));
    if(cmd == MS5611_D1_OSR_4096) {
        D1 = ((uint32_t) data[0] << 16) | (uint32_t) (data[1] << 8) | (uint32_t) data[2];
    } else if(cmd == MS5611_D2_OSR_4096) {
        D2 = ((uint32_t) data[0] << 16) | (uint32_t) (data[1] << 8) | (uint32_t) data[2];
    } else {
         ESP_LOGE(TAG, "Invalid command for conversion: 0x%02X", cmd);
     }
}

int32_t ms5611_calculateTemperature()
{   
    dT = D2 - (((uint32_t) prom_data[5])<<8);
    // int64_t calcTemp = 2000 + ((int64_t)dT * prom_data[6]) / (1LL << 23);
    int32_t TEMP = (int32_t) (2000 + ((int64_t)dT * prom_data[6]) / (1LL << 23));

    T2 = 0;
    OFF2 = 0;
    SENS2 = 0;
    //Compensate for low Temp
    if(TEMP<2000)
    {
        T2 = (int32_t)(((int64_t)dT * dT) >> 31);
        OFF2 = 5 * ((int64_t)TEMP-2000) * ((int64_t)TEMP-2000)/2;
        SENS2 = 5 * ((int64_t)TEMP-2000) * ((int64_t)TEMP-2000)/4;
    }

    if(TEMP<-1500)
    {
        OFF2 += 7 * ((int64_t)+1500) * ((int64_t)TEMP+1500);
        SENS2 += 11 * ((int64_t)TEMP+1500) * ((int64_t)TEMP+1500)/2;
    }

    //Final Temp result
    TEMP -= T2;
    return TEMP;
}

int32_t ms5611_calculatePressure()
{
    OFF  = ((int64_t)prom_data[2] << 16) + ((int64_t)prom_data[4] * dT) / (1LL << 7);
    SENS = ((int64_t)prom_data[1] << 15) + ((int64_t)prom_data[3] * dT) / (1LL << 8);

    OFF  -= OFF2;
    SENS -= SENS2;

    int32_t P = (int32_t)(((((int64_t)D1 * SENS) / (1LL << 21)) - OFF) / (1LL << 15));
    return P;
}
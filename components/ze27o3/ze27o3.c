#include "ze27o3.h"

/* ---- ZE27O3 commands and parsing ----
    Following functions implement the ZE27O3 sensor communication protocol */

esp_err_t ze27_uart_init(void){
    uart_config_t uart2_config = {
        .baud_rate = ZE27O3_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(ZE27O3_UART_NUM, ZE27O3_UART_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(ZE27O3_UART_NUM, &uart2_config));
    ESP_ERROR_CHECK(uart_set_pin(
        ZE27O3_UART_NUM,
        ZE27O3_UART_TX_PIN,
        ZE27O3_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    ));

    return ESP_OK;  
}

uint8_t ze27_checksum(const uint8_t *buf, size_t len){
    uint8_t sum = 0;
    for (size_t i = 1; i < len - 1; i++) {
        sum += buf[i];
    }
    return (uint8_t)(~sum + 1);
}

esp_err_t ze27o3_readActiveUpload(uint16_t *o3_ppb)
{
    if (o3_ppb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t rxbuf[ZE27O3_UART_BUF_SIZE];
    uint8_t frame[9];
    size_t frame_pos = 0;

    int len = uart_read_bytes(
        ZE27O3_UART_NUM,
        rxbuf,
        sizeof(rxbuf),
        pdMS_TO_TICKS(1000)
    );

    if (len == 0) {
    return ESP_ERR_TIMEOUT;  // normal
    }
    
    if (len < 0) {
        return ESP_FAIL;         // actual error
    }

    for (int i = 0; i < len; i++) {
        uint8_t b = rxbuf[i];

        if (frame_pos == 0) {
            if (b != 0xFF) {
                continue; // wait for start byte
            }
        }

        frame[frame_pos++] = b;

        if (frame_pos == 9) {
            if (frame[0] != 0xFF) {
                frame_pos = 0;
                continue;
            }

            uint8_t expected = ze27_checksum(frame, 9);
            if (frame[8] != expected) {
                ESP_LOGE("ZE27O3", "Checksum mismatch: expected 0x%02X, got 0x%02X",
                         expected, frame[8]);
                frame_pos = 0;
                continue;
            }

            if (frame[1] != 0x86) {
                ESP_LOGE("ZE27O3", "Unexpected command byte: 0x%02X", frame[1]);
                frame_pos = 0;
                continue;
            }

            uint16_t ppb = ((uint16_t)frame[2] << 8) | frame[3];
            *o3_ppb = ppb;

            ESP_LOGI("ZE27O3", "O3 Concentration: %u ppb", ppb);
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}
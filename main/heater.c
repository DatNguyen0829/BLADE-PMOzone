#include "heater.h"

void heat_pwm_init(void){
    ledc_timer_config_t timer_cfg = {
        .speed_mode       = HEAT_LEDC_MODE,
        .duty_resolution  = HEAT_LEDC_DUTY_RES,
        .timer_num        = HEAT_LEDC_TIMER,
        .freq_hz          = HEAT_LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t chan_cfg = {
        .gpio_num   = HEAT_GATE,
        .speed_mode = HEAT_LEDC_MODE,
        .channel    = HEAT_LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = HEAT_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan_cfg));
}

void heat_set_duty_percent(uint8_t duty_percent){
    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;

    uint32_t duty = (HEAT_LEDC_MAX_DUTY * duty_percent) / 100;

    ESP_ERROR_CHECK(ledc_set_duty(HEAT_LEDC_MODE, HEAT_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(HEAT_LEDC_MODE, HEAT_LEDC_CHANNEL));
}

#include "driver/ledc.h"

#define HEAT_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define HEAT_LEDC_TIMER      LEDC_TIMER_0
#define HEAT_LEDC_CHANNEL    LEDC_CHANNEL_0
#define HEAT_LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define HEAT_LEDC_FREQ_HZ    10
#define HEAT_LEDC_MAX_DUTY   ((1 << 10) - 1)   // 1023 for 10-bit
#define HEAT_GATE 26 // GPIO to turn on heating pad


void heat_pwm_init(void);
void heat_set_duty_percent(int duty_percent);

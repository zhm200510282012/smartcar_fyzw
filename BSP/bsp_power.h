#ifndef BSP_POWER_H
#define BSP_POWER_H

#include "../App/app_types.h"

typedef struct {
    u16 raw_adc;
    u16 filtered_adc;
    u16 voltage_estimate_mv;
    app_bool_t calibration_valid;
    app_bool_t power_good;
    app_bool_t power_uncalibrated;
    app_bool_t adc_valid;
} power_sample_t;

void bsp_power_init(void);
power_sample_t bsp_power_read_sample(void);
power_sample_t bsp_power_last_sample(void);
app_bool_t bsp_power_is_ok(void);

#ifdef HOST_SIL
void bsp_power_host_set_raw(u16 raw_adc, app_bool_t adc_valid);
#endif

#endif

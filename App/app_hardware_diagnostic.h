#ifndef APP_HARDWARE_DIAGNOSTIC_H
#define APP_HARDWARE_DIAGNOSTIC_H

#include "app_types.h"
#include "../BSP/bsp_power.h"

typedef struct {
    power_sample_t power;
    app_bool_t arm_requested;
    emag_sample_t emag;
    attitude_sample_t attitude;
    encoder_sample_t encoder;
    u8 route_progress_status;
} app_hardware_diagnostic_snapshot_t;

void app_hardware_diagnostic_init(void);
app_hardware_diagnostic_snapshot_t app_hardware_diagnostic_run_once(void);

#endif

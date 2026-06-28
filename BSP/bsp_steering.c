#include "bsp_steering.h"
#include "../App/app_config.h"
#include "board_map.h"

static u16 g_steering_pulse_us;

void bsp_steering_init(void)
{
    g_steering_pulse_us = STEERING_SAFE_CENTER_US;
}

void bsp_steering_apply(u16 steering_pulse_us)
{
    if (BOARD_STEERING_VERIFIED == 0) {
        g_steering_pulse_us = STEERING_SAFE_CENTER_US;
        return;
    }
    if (steering_pulse_us < STEERING_MIN_PULSE_US) steering_pulse_us = STEERING_MIN_PULSE_US;
    if (steering_pulse_us > STEERING_MAX_PULSE_US) steering_pulse_us = STEERING_MAX_PULSE_US;
    g_steering_pulse_us = steering_pulse_us;
}

u16 bsp_steering_last_pulse_us(void)
{
    return g_steering_pulse_us;
}

#ifndef BSP_CONTROL_TIMERS_H
#define BSP_CONTROL_TIMERS_H

#include "../App/app_types.h"

#define BSP_SENSOR_TIMER_NAME "Timer1"
#define BSP_CONTROL_TIMER_NAME "Timer11"
#define BSP_SENSOR_TIMER_PRIORITY 3u
#define BSP_CONTROL_TIMER_PRIORITY 2u

typedef struct {
    const char *sensor_timer;
    const char *control_timer;
    u16 sensor_hz;
    u16 control_hz;
    u8 sensor_priority;
    u8 control_priority;
} bsp_control_timer_config_t;

void bsp_control_timers_init(void);
bsp_control_timer_config_t bsp_control_timers_config(void);

#endif

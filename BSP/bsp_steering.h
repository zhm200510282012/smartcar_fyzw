#ifndef BSP_STEERING_H
#define BSP_STEERING_H

#include "../App/app_types.h"

void bsp_steering_init(void);
void bsp_steering_apply(u16 steering_pulse_us);
void bsp_steering_apply_pair(u16 left_pulse_us, u16 right_pulse_us);
u16 bsp_steering_last_pulse_us(void);
u16 bsp_steering_last_left_pulse_us(void);
u16 bsp_steering_last_right_pulse_us(void);

#endif

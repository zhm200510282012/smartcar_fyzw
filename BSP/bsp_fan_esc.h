#ifndef BSP_FAN_ESC_H
#define BSP_FAN_ESC_H

#include "../App/app_types.h"

void bsp_fan_esc_init(void);
void bsp_fan_esc_apply(const fan_esc_command_t *cmd);
void bsp_fan_esc_force_off(void);
fan_esc_command_t bsp_fan_esc_last_command(void);
u16 bsp_fan_esc_last_output_us(void);

#endif

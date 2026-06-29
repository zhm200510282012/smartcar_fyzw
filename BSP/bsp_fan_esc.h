#ifndef BSP_FAN_ESC_H
#define BSP_FAN_ESC_H

/*
 * 模块职责：P2.2 / PWM2P_3 无刷风机 ESC 输出层。
 * 输入：fan_esc_command_t 的逻辑状态和 request_us。
 * 输出：真实 PWM 脉宽；FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0 时真实输出强制为 0。
 */

#include "../App/app_types.h"

void bsp_fan_esc_init(void);
void bsp_fan_esc_apply(const fan_esc_command_t *cmd);
void bsp_fan_esc_force_off(void);
fan_esc_command_t bsp_fan_esc_last_command(void);
u16 bsp_fan_esc_last_output_us(void);

#endif

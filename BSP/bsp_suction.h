#ifndef BSP_SUCTION_H
#define BSP_SUCTION_H

#include "../App/app_types.h"

void bsp_suction_init(void);
void bsp_suction_apply(const suction_command_t *cmd);
suction_command_t bsp_suction_last_command(void);
u16 bsp_suction_last_native_output(void);

#endif

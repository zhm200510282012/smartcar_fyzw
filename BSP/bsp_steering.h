#ifndef BSP_STEERING_H
#define BSP_STEERING_H

#include "../App/app_types.h"

void bsp_steering_init(void);
void bsp_steering_apply(s16 command);
s16 bsp_steering_last_command(void);

#endif

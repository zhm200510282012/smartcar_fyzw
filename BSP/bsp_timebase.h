#ifndef BSP_TIMEBASE_H
#define BSP_TIMEBASE_H

#include "../App/app_types.h"

void bsp_timebase_init(void);
u32 bsp_timebase_now_ms(void);
void bsp_timebase_on_tick_1ms(void);

#endif

#ifndef BSP_TIMING_SCOPE_H
#define BSP_TIMING_SCOPE_H

#include "../App/app_types.h"

#define BSP_TIMING_SCOPE_ENABLE 0

void bsp_timing_scope_sensor_enter(void);
void bsp_timing_scope_sensor_exit(void);
void bsp_timing_scope_control_enter(void);
void bsp_timing_scope_control_exit(void);

#ifdef HOST_SIL
u16 bsp_timing_scope_debug_transition_count(void);
#endif

#endif

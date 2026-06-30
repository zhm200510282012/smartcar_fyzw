#include "bsp_timing_scope.h"

#ifdef HOST_SIL
static u16 g_scope_transition_count;
#endif

static void scope_noop(void)
{
#if BSP_TIMING_SCOPE_ENABLE != 0
    /*
     * Intentionally empty until a real spare oscilloscope GPIO is assigned.
     * Do not add a pin write here without updating the hardware map.
     */
#endif
}

void bsp_timing_scope_sensor_enter(void)
{
    scope_noop();
}

void bsp_timing_scope_sensor_exit(void)
{
    scope_noop();
}

void bsp_timing_scope_control_enter(void)
{
    scope_noop();
}

void bsp_timing_scope_control_exit(void)
{
    scope_noop();
}

#ifdef HOST_SIL
u16 bsp_timing_scope_debug_transition_count(void)
{
    return g_scope_transition_count;
}
#endif

#include "bsp_timebase.h"

static volatile u32 g_ms;

void bsp_timebase_init(void)
{
    g_ms = 0ul;
}

u32 bsp_timebase_now_ms(void)
{
    return g_ms;
}

void bsp_timebase_on_tick_1ms(void)
{
    g_ms++;
}

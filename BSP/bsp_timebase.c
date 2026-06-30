#include "../App/app_config.h"
#include "../App/app_control_tick.h"
#include "bsp_control_timers.h"
#include "bsp_timing_scope.h"

#ifndef HOST_SIL
#include "AI8051U_NVIC.h"
#include "AI8051U_Timer.h"
#endif
#include "bsp_timebase.h"

#define TIMEBASE_CONTROL_DIVIDER (TIMEBASE_TICK_HZ / CONTROL_PID_HZ)

#if ((TIMEBASE_TICK_HZ % CONTROL_PID_HZ) != 0u)
#error CONTROL_PID_HZ must divide TIMEBASE_TICK_HZ
#endif

static volatile u32 g_ms;
static u8 g_control_divider;

void bsp_timebase_init(void)
{
    g_ms = 0ul;
    g_control_divider = 0u;
#ifndef HOST_SIL
    {
        TIM_InitTypeDef tim;
        tim.TIM_Mode = TIM_16BitAutoReload;
        tim.TIM_ClkSource = TIM_CLOCK_1T;
        tim.TIM_ClkOut = DISABLE;
        tim.TIM_Value = (u16)(65536ul - (FYZW_MAIN_FOSC_HZ / (u32)TIMEBASE_TICK_HZ));
        tim.TIM_PS = 0u;
        tim.TIM_Run = ENABLE;
        Timer_Inilize(Timer0, &tim);
        NVIC_Timer0_Init(ENABLE, BSP_CONTROL_TIMER_PRIORITY);
        Global_IRQ_Enable();
    }
#endif
}

u32 bsp_timebase_now_ms(void)
{
    return g_ms;
}

void bsp_timebase_on_tick_1ms(void)
{
    g_ms++;
}

#ifndef HOST_SIL
void bsp_timebase_timer0_isr(void) interrupt TMR0_VECTOR
{
    app_context_t *ctx;

    g_ms++;
    g_control_divider++;
    if (g_control_divider >= TIMEBASE_CONTROL_DIVIDER) {
        g_control_divider = 0u;
        ctx = app_control_tick_bound_context();
        if (ctx != 0) {
            bsp_timing_scope_control_enter();
            app_control_tick_control_isr(ctx, g_ms);
            bsp_timing_scope_control_exit();
        }
    }
}
#endif

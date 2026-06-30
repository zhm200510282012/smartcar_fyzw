#include "bsp_control_timers.h"
#include "bsp_timebase.h"
#include "bsp_timing_scope.h"
#include "../App/app_config.h"
#include "../App/app_control_tick.h"

#ifndef HOST_SIL
#include "AI8051U_NVIC.h"
#include "AI8051U_Timer.h"
#endif

static bsp_control_timer_config_t g_timer_config;

#ifndef HOST_SIL
static u16 timer_reload_for_hz(u16 hz)
{
    u32 ticks;

    if (hz == 0u) {
        return 0u;
    }
    ticks = FYZW_MAIN_FOSC_HZ / (u32)hz;
    if (ticks > 65535ul) {
        ticks = 65535ul;
    }
    return (u16)(65536ul - ticks);
}
#endif

void bsp_control_timers_init(void)
{
    g_timer_config.sensor_timer = BSP_SENSOR_TIMER_NAME;
    g_timer_config.control_timer = BSP_CONTROL_TIMER_NAME;
    g_timer_config.sensor_hz = SENSOR_ADC_TICK_HZ;
    g_timer_config.control_hz = CONTROL_PID_HZ;
    g_timer_config.sensor_priority = BSP_SENSOR_TIMER_PRIORITY;
    g_timer_config.control_priority = BSP_CONTROL_TIMER_PRIORITY;

#ifndef HOST_SIL
    {
        TIM_InitTypeDef tim;

        tim.TIM_Mode = TIM_16BitAutoReload;
        tim.TIM_ClkSource = TIM_CLOCK_1T;
        tim.TIM_ClkOut = DISABLE;
        tim.TIM_PS = 0u;
        tim.TIM_Run = ENABLE;

        tim.TIM_Value = timer_reload_for_hz(SENSOR_ADC_TICK_HZ);
        Timer_Inilize(Timer1, &tim);
        NVIC_Timer1_Init(ENABLE, BSP_SENSOR_TIMER_PRIORITY);
    }
#endif
}

bsp_control_timer_config_t bsp_control_timers_config(void)
{
    return g_timer_config;
}

#ifndef HOST_SIL
void bsp_sensor_timer1_isr(void) interrupt TMR1_VECTOR
{
    app_context_t *ctx;

    bsp_timing_scope_sensor_enter();
    ctx = app_control_tick_bound_context();
    if (ctx != 0) {
        app_control_tick_sensor_isr(ctx, bsp_timebase_now_ms());
    }
    bsp_timing_scope_sensor_exit();
}
#endif

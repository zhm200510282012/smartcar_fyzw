#ifndef HOST_SIL
#include "../App/app_config.h"
#include "AI8051U_NVIC.h"
#include "AI8051U_Timer.h"
#endif
#include "bsp_timebase.h"

static volatile u32 g_ms;

void bsp_timebase_init(void)
{
    g_ms = 0ul;
#ifndef HOST_SIL
    {
        TIM_InitTypeDef tim;
        tim.TIM_Mode = TIM_16BitAutoReload;
        tim.TIM_ClkSource = TIM_CLOCK_1T;
        tim.TIM_ClkOut = DISABLE;
        tim.TIM_Value = (u16)(65536ul - (FYZW_MAIN_FOSC_HZ / 1000ul));
        tim.TIM_PS = 0u;
        tim.TIM_Run = ENABLE;
        Timer_Inilize(Timer0, &tim);
        NVIC_Timer0_Init(ENABLE, Priority_1);
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
    g_ms++;
}
#endif

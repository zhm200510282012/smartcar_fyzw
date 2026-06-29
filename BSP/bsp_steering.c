#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_PWM.h"
#endif
#include "bsp_steering.h"
#include "../App/app_config.h"
#include "board_map.h"

static u16 g_steering_pulse_us;
static u16 g_steering_left_pulse_us;
static u16 g_steering_right_pulse_us;

static u16 clamp_pulse(u16 pulse_us)
{
    if (pulse_us < STEERING_MIN_PULSE_US) return STEERING_MIN_PULSE_US;
    if (pulse_us > STEERING_MAX_PULSE_US) return STEERING_MAX_PULSE_US;
    return pulse_us;
}

#ifndef HOST_SIL
static void steering_hw_disable_outputs(void)
{
    UpdatePwmCh(PWM1, STEERING_SAFE_CENTER_US);
    UpdatePwmCh(PWM2, STEERING_SAFE_CENTER_US);
    PWM1P_OUT_DIS();
    PWM2P_OUT_DIS();
}

static void steering_hw_init(void)
{
    PWMx_InitDefine pwm;

    PWMA_Prescaler((u16)(FYZW_MAIN_FOSC_HZ / 1000000ul - 1ul));
    PWM1_USE_P10P11();
    PWM2_USE_P12P13();
    GPIO_Init(GPIO_P1, (u8)(GPIO_Pin_0 | GPIO_Pin_2), GPIO_Mode_Out_PP);

    pwm.PWM_Mode = CCMRn_PWM_MODE1;
    pwm.PWM_Duty = STEERING_SAFE_CENTER_US;
    pwm.PWM_DeadTime = 0u;
    pwm.PWM_EnoSelect = ENO1P;
    pwm.PWM_CEN_Enable = ENABLE;
    pwm.PWM_MainOutEnable = ENABLE;
    PWM_Configuration(PWM1, &pwm);

    pwm.PWM_EnoSelect = ENO2P;
    PWM_Configuration(PWM2, &pwm);

    pwm.PWM_Period = (u16)(1000000ul / STEERING_PWM_FREQ_HZ);
    pwm.PWM_EnoSelect = (u8)(ENO1P | ENO2P);
    PWM_Configuration(PWMA, &pwm);
    steering_hw_disable_outputs();
}

static void steering_hw_write(u16 left_pulse_us, u16 right_pulse_us)
{
    if (BOARD_STEERING_VERIFIED == 0) {
        steering_hw_disable_outputs();
        return;
    }
    UpdatePwmCh(PWM1, left_pulse_us);
    UpdatePwmCh(PWM2, right_pulse_us);
    PWM1P_OUT_EN();
    PWM2P_OUT_EN();
}
#endif

void bsp_steering_init(void)
{
    g_steering_pulse_us = STEERING_SAFE_CENTER_US;
    g_steering_left_pulse_us = STEERING_SAFE_CENTER_US;
    g_steering_right_pulse_us = STEERING_SAFE_CENTER_US;
#ifndef HOST_SIL
    steering_hw_init();
#endif
}

void bsp_steering_apply(u16 steering_pulse_us)
{
    bsp_steering_apply_pair(steering_pulse_us, steering_pulse_us);
}

void bsp_steering_apply_pair(u16 left_pulse_us, u16 right_pulse_us)
{
    if (BOARD_STEERING_VERIFIED == 0) {
        g_steering_pulse_us = STEERING_SAFE_CENTER_US;
        g_steering_left_pulse_us = STEERING_SAFE_CENTER_US;
        g_steering_right_pulse_us = STEERING_SAFE_CENTER_US;
#ifndef HOST_SIL
        steering_hw_disable_outputs();
#endif
        return;
    }
    left_pulse_us = clamp_pulse(left_pulse_us);
    right_pulse_us = clamp_pulse(right_pulse_us);
    g_steering_left_pulse_us = left_pulse_us;
    g_steering_right_pulse_us = right_pulse_us;
    g_steering_pulse_us = (u16)((left_pulse_us + right_pulse_us) / 2u);
#ifndef HOST_SIL
    steering_hw_write(left_pulse_us, right_pulse_us);
#endif
}

u16 bsp_steering_last_pulse_us(void)
{
    return g_steering_pulse_us;
}

u16 bsp_steering_last_left_pulse_us(void)
{
    return g_steering_left_pulse_us;
}

u16 bsp_steering_last_right_pulse_us(void)
{
    return g_steering_right_pulse_us;
}

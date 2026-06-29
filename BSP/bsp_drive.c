#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_PWM.h"
#endif
#include "bsp_drive.h"
#include "../App/app_config.h"
#include "board_map.h"

static s16 g_drive_command_native;
static s16 g_drive_left_native;
static s16 g_drive_right_native;

static s16 clamp_drive(s16 value)
{
    if (value > DRIVE_LIMIT_ABS) return DRIVE_LIMIT_ABS;
    if (value < -DRIVE_LIMIT_ABS) return -DRIVE_LIMIT_ABS;
    return value;
}

static u16 abs_drive(s16 value)
{
    if (value < 0) {
        return (u16)(0 - value);
    }
    return (u16)value;
}

#ifndef HOST_SIL
static void drive_hw_disable_outputs(void)
{
    PWM5P_OUT_DIS();
    PWM7P_OUT_DIS();
    UpdatePwmCh(PWM5, 0u);
    UpdatePwmCh(PWM7, 0u);
    gpio_write_pin(P5_1, 0u);
    gpio_write_pin(P5_3, 0u);
}

static void drive_hw_init(void)
{
    PWMx_InitDefine pwm;
    u16 period;

    period = (u16)(FYZW_MAIN_FOSC_HZ / DRIVE_PWM_FREQ_HZ);
    if (period == 0u) {
        period = 1u;
    }

    PWMB_Prescaler(0u);
    PWM5_USE_P50();
    PWM7_USE_P52();
    GPIO_Init(GPIO_P5, (u8)(GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3), GPIO_Mode_Out_PP);

    pwm.PWM_Mode = CCMRn_PWM_MODE1;
    pwm.PWM_Duty = 0u;
    pwm.PWM_DeadTime = 0u;
    pwm.PWM_EnoSelect = ENO5P;
    pwm.PWM_CEN_Enable = ENABLE;
    pwm.PWM_MainOutEnable = ENABLE;
    PWM_Configuration(PWM5, &pwm);

    pwm.PWM_EnoSelect = ENO7P;
    PWM_Configuration(PWM7, &pwm);

    pwm.PWM_Period = period;
    pwm.PWM_EnoSelect = (u8)(ENO5P | ENO7P);
    PWM_Configuration(PWMB, &pwm);
    drive_hw_disable_outputs();
}

static void drive_hw_write(s16 left_native, s16 right_native)
{
    if (BOARD_DRIVE_VERIFIED == 0) {
        drive_hw_disable_outputs();
        return;
    }

    if (left_native < 0) {
        gpio_write_pin(P5_1, 1u);
    } else {
        gpio_write_pin(P5_1, 0u);
    }
    if (right_native < 0) {
        gpio_write_pin(P5_3, 1u);
    } else {
        gpio_write_pin(P5_3, 0u);
    }
    UpdatePwmCh(PWM5, abs_drive(left_native));
    UpdatePwmCh(PWM7, abs_drive(right_native));
    PWM5P_OUT_EN();
    PWM7P_OUT_EN();
}
#endif

void bsp_drive_init(void)
{
    g_drive_command_native = DRIVE_SAFE_ZERO;
    g_drive_left_native = DRIVE_SAFE_ZERO;
    g_drive_right_native = DRIVE_SAFE_ZERO;
#ifndef HOST_SIL
    drive_hw_init();
#endif
}

void bsp_drive_apply(s16 drive_command_native)
{
    bsp_drive_apply_lr(drive_command_native, drive_command_native);
}

void bsp_drive_apply_lr(s16 left_native, s16 right_native)
{
    if (BOARD_DRIVE_VERIFIED == 0) {
        g_drive_command_native = DRIVE_SAFE_ZERO;
        g_drive_left_native = DRIVE_SAFE_ZERO;
        g_drive_right_native = DRIVE_SAFE_ZERO;
#ifndef HOST_SIL
        drive_hw_disable_outputs();
#endif
        return;
    }
    left_native = clamp_drive(left_native);
    right_native = clamp_drive(right_native);
    g_drive_left_native = left_native;
    g_drive_right_native = right_native;
    g_drive_command_native = (s16)((left_native + right_native) / 2);
#ifndef HOST_SIL
    drive_hw_write(left_native, right_native);
#endif
}

s16 bsp_drive_last_command_native(void)
{
    return g_drive_command_native;
}

s16 bsp_drive_last_left_native(void)
{
    return g_drive_left_native;
}

s16 bsp_drive_last_right_native(void)
{
    return g_drive_right_native;
}

/*
 * 模块职责：AI8051U PWM2P_3(P2.2) 风机 ESC BSP。
 * Host-SIL 只记录最后命令；真实 C251 使用 AI8051U PWM API 初始化 50 Hz 脉冲。
 * 物理总开关关闭时，即使逻辑 request_us 变化，也禁能 PWM 并把 P2.2 拉到安全输出。
 */
#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_PWM.h"
#endif
#include "bsp_fan_esc.h"
#include "../App/app_config.h"
#include "board_map.h"

static fan_esc_command_t g_last_command;
static u16 g_last_output_us;
static u16 g_last_applied_us;

static u16 clamp_pulse_us(u16 value)
{
    if (value < FAN_ESC_MIN_US) return FAN_ESC_MIN_US;
    if (value > FAN_ESC_MAX_US) return FAN_ESC_MAX_US;
    return value;
}

static u8 physical_output_allowed(void)
{
    if (BOARD_FAN_ESC_SIGNAL_MAPPED == 0) return APP_FALSE;
    if (BOARD_FAN_ESC_BENCH_VERIFIED == 0) return APP_FALSE;
    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE == 0) return APP_FALSE;
    return APP_TRUE;
}

#ifndef HOST_SIL
static void fan_hw_disable_output(void)
{
    PWM2P_OUT_DIS();
    UpdatePwmCh(PWM2, 0u);
    gpio_write_pin(P2_2, 0u);
}

static void fan_hw_init(void)
{
    PWMx_InitDefine pwm;
    u16 period;
    u16 prescaler;

    prescaler = (u16)((FYZW_MAIN_FOSC_HZ / 1000000ul) - 1ul);
    period = (u16)(1000000ul / (u32)FAN_PWM_FREQ_HZ);

    PWMA_Prescaler(prescaler);
    PWM2_USE_P22P23();
    GPIO_Init(GPIO_P2, GPIO_Pin_2, GPIO_Mode_Out_PP);

    pwm.PWM_Mode = CCMRn_PWM_MODE1;
    pwm.PWM_Duty = 0u;
    pwm.PWM_DeadTime = 0u;
    pwm.PWM_EnoSelect = ENO2P;
    pwm.PWM_CEN_Enable = ENABLE;
    pwm.PWM_MainOutEnable = ENABLE;
    PWM_Configuration(PWM2, &pwm);

    pwm.PWM_Period = period;
    pwm.PWM_EnoSelect = ENO2P;
    PWM_Configuration(PWMA, &pwm);
    fan_hw_disable_output();
}

static void fan_hw_write(u16 pulse_us)
{
    UpdatePwmCh(PWM2, pulse_us);
    PWM2P_OUT_EN();
}
#endif

static void store_off_command(void)
{
    g_last_command.state = FAN_ESC_OFF;
    g_last_command.request_us = FAN_ESC_MIN_US;
    g_last_command.output_us = 0u;
    g_last_command.mapped = BOARD_FAN_ESC_SIGNAL_MAPPED;
    g_last_command.physical_enabled = APP_FALSE;
    g_last_command.bench_verified = BOARD_FAN_ESC_BENCH_VERIFIED;
    g_last_output_us = 0u;
    g_last_applied_us = 0u;
}

void bsp_fan_esc_init(void)
{
#ifndef HOST_SIL
    fan_hw_init();
#endif
    store_off_command();
}

void bsp_fan_esc_force_off(void)
{
    store_off_command();
#ifndef HOST_SIL
    fan_hw_disable_output();
#endif
}

void bsp_fan_esc_apply(const fan_esc_command_t *cmd)
{
    fan_esc_command_t safe;

    if (cmd == 0) {
        bsp_fan_esc_force_off();
        return;
    }

    safe = *cmd;
    safe.mapped = BOARD_FAN_ESC_SIGNAL_MAPPED;
    safe.physical_enabled = physical_output_allowed();
    safe.bench_verified = BOARD_FAN_ESC_BENCH_VERIFIED;
    safe.request_us = clamp_pulse_us(safe.request_us);

    if (safe.state == FAN_ESC_OFF || safe.physical_enabled == APP_FALSE) {
        safe.output_us = 0u;
        g_last_output_us = 0u;
        g_last_applied_us = 0u;
        g_last_command = safe;
#ifndef HOST_SIL
        fan_hw_disable_output();
#endif
        return;
    }

    safe.output_us = safe.request_us;
    g_last_output_us = safe.output_us;
    g_last_applied_us = safe.output_us;
    g_last_command = safe;
#ifndef HOST_SIL
    fan_hw_write(safe.output_us);
#endif
}

fan_esc_command_t bsp_fan_esc_last_command(void)
{
    return g_last_command;
}

u16 bsp_fan_esc_last_output_us(void)
{
    return g_last_output_us;
}

u16 bsp_fan_esc_last_applied_us(void)
{
    return g_last_applied_us;
}

app_bool_t bsp_fan_esc_is_physical_active(void)
{
    if (g_last_command.physical_enabled == APP_FALSE) {
        return APP_FALSE;
    }
    return (g_last_applied_us > 0u) ? APP_TRUE : APP_FALSE;
}

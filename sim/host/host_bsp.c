#include "host_bsp.h"
#include "../../App/app_config.h"
#include "../../BSP/bsp_drive.h"
#include "../../BSP/bsp_emag.h"
#include "../../BSP/bsp_encoder.h"
#include "../../BSP/bsp_imu.h"
#include "../../BSP/bsp_power.h"
#include "../../BSP/bsp_steering.h"
#include "../../BSP/bsp_suction.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"

static u32 g_now_ms;
static host_sil_input_t g_input;
static s16 g_drive_command_native;
static u16 g_steering_pulse_us;
static suction_command_t g_suction_command;
static u16 g_suction_native_output;
static app_state_t g_last_ui_state;

void host_bsp_reset(void)
{
    g_now_ms = 0ul;
    g_input.time_ms = 0ul;
    g_input.manual_arm = APP_FALSE;
    g_input.suction_authorize = APP_FALSE;
    g_input.transition_candidate = APP_FALSE;
    g_input.emag_valid = APP_TRUE;
    g_input.line_error = 0;
    g_input.signal_quality = LINE_QUALITY_MIN;
    g_input.imu_fresh = APP_TRUE;
    g_input.pitch_cdeg = 0;
    g_input.encoder_valid = APP_TRUE;
    g_input.left_count = 0l;
    g_input.right_count = 0l;
    g_input.left_speed_mm_s = 0;
    g_input.right_speed_mm_s = 0;
    g_input.power_ok = APP_TRUE;
    g_drive_command_native = DRIVE_SAFE_ZERO;
    g_steering_pulse_us = STEERING_SAFE_CENTER_US;
    g_suction_command.mode = SUCTION_OFF;
    g_suction_command.command_native = SUCTION_SAFE_OFF_NATIVE;
    g_suction_command.armed = APP_FALSE;
    g_suction_command.hw_verified = SUCTION_HW_VERIFIED;
    g_suction_command.feedback_valid = APP_FALSE;
    g_suction_command.fault_code = FAULT_NONE;
    g_suction_native_output = SUCTION_SAFE_OFF_NATIVE;
    g_last_ui_state = APP_STATE_BOOT;
}

void host_bsp_set_input(const host_sil_input_t *input)
{
    if (input != 0) {
        g_input = *input;
    }
}

u8 host_bsp_transition_candidate(void)
{
    return g_input.transition_candidate;
}

void bsp_timebase_init(void)
{
    g_now_ms = 0ul;
}

u32 bsp_timebase_now_ms(void)
{
    return g_now_ms;
}

void bsp_timebase_on_tick_1ms(void)
{
    g_now_ms++;
}

void bsp_drive_init(void)
{
    g_drive_command_native = DRIVE_SAFE_ZERO;
}

void bsp_drive_apply(s16 drive_command_native)
{
    if (drive_command_native > DRIVE_LIMIT_ABS) drive_command_native = DRIVE_LIMIT_ABS;
    if (drive_command_native < -DRIVE_LIMIT_ABS) drive_command_native = -DRIVE_LIMIT_ABS;
    g_drive_command_native = drive_command_native;
}

s16 bsp_drive_last_command_native(void)
{
    return g_drive_command_native;
}

void bsp_steering_init(void)
{
    g_steering_pulse_us = STEERING_SAFE_CENTER_US;
}

void bsp_steering_apply(u16 steering_pulse_us)
{
    if (steering_pulse_us < STEERING_MIN_PULSE_US) steering_pulse_us = STEERING_MIN_PULSE_US;
    if (steering_pulse_us > STEERING_MAX_PULSE_US) steering_pulse_us = STEERING_MAX_PULSE_US;
    g_steering_pulse_us = steering_pulse_us;
}

u16 bsp_steering_last_pulse_us(void)
{
    return g_steering_pulse_us;
}

void bsp_suction_init(void)
{
    g_suction_command.mode = SUCTION_OFF;
    g_suction_command.command_native = SUCTION_SAFE_OFF_NATIVE;
    g_suction_command.armed = APP_FALSE;
    g_suction_command.hw_verified = SUCTION_HW_VERIFIED;
    g_suction_command.feedback_valid = APP_FALSE;
    g_suction_command.fault_code = FAULT_NONE;
    g_suction_native_output = SUCTION_SAFE_OFF_NATIVE;
}

void bsp_suction_apply(const suction_command_t *cmd)
{
    if (cmd == 0) {
        bsp_suction_init();
        return;
    }
    g_suction_command = *cmd;
#if SUCTION_HW_VERIFIED == 0
    g_suction_command.hw_verified = APP_FALSE;
    g_suction_command.fault_code = (u8)(g_suction_command.fault_code | FAULT_SUCTION_UNVERIFIED);
    g_suction_native_output = SUCTION_SAFE_OFF_NATIVE;
#else
    g_suction_command.hw_verified = APP_TRUE;
    g_suction_native_output = g_suction_command.command_native;
#endif
}

suction_command_t bsp_suction_last_command(void)
{
    return g_suction_command;
}

u16 bsp_suction_last_native_output(void)
{
    return g_suction_native_output;
}

void bsp_emag_init(void)
{
}

emag_sample_t bsp_emag_read(void)
{
    emag_sample_t sample;
    u16 half_quality;
    half_quality = (u16)(g_input.signal_quality / 2u);
    sample.raw[0] = half_quality;
    sample.raw[1] = half_quality;
    sample.raw[2] = half_quality;
    sample.raw[3] = half_quality;
    sample.raw[4] = half_quality;
    sample.filtered[0] = half_quality;
    sample.filtered[1] = half_quality;
    sample.filtered[2] = half_quality;
    sample.filtered[3] = half_quality;
    sample.filtered[4] = half_quality;
    sample.norm[0] = (u16)(half_quality - (g_input.line_error / 2));
    sample.norm[1] = half_quality;
    sample.norm[2] = half_quality;
    sample.norm[3] = half_quality;
    sample.norm[4] = (u16)(half_quality + (g_input.line_error / 2));
    sample.line_error = g_input.line_error;
    sample.signal_quality = g_input.signal_quality;
    sample.channel_count = 5u;
    sample.valid = g_input.emag_valid;
    return sample;
}

void bsp_imu_init(void)
{
}

attitude_sample_t bsp_imu_read(void)
{
    attitude_sample_t sample;
    sample.roll_cdeg = 0;
    sample.pitch_cdeg = g_input.pitch_cdeg;
    sample.yaw_rate_cdeg_s = 0;
    sample.timestamp_ms = g_input.time_ms;
    sample.id_ok = APP_TRUE;
    sample.fresh = g_input.imu_fresh;
    return sample;
}

void bsp_encoder_init(void)
{
}

encoder_sample_t bsp_encoder_read(void)
{
    encoder_sample_t sample;
    sample.left_count = g_input.left_count;
    sample.right_count = g_input.right_count;
    sample.left_speed_mm_s = g_input.left_speed_mm_s;
    sample.right_speed_mm_s = g_input.right_speed_mm_s;
    sample.valid = g_input.encoder_valid;
    return sample;
}

void bsp_power_init(void)
{
}

u8 bsp_power_is_ok(void)
{
    return g_input.power_ok;
}

void bsp_ui_init(void)
{
    g_last_ui_state = APP_STATE_BOOT;
}

u8 bsp_ui_manual_arm_requested(void)
{
    return g_input.manual_arm;
}

u8 bsp_ui_suction_authorized(void)
{
    return g_input.suction_authorize;
}

void bsp_ui_show_state(app_state_t state)
{
    g_last_ui_state = state;
}

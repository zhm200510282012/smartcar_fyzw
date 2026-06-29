#include "app_output_arbitration.h"
#include "app_config.h"
#include "app_safety.h"

static s16 clamp_drive(s16 value)
{
    if (value > DRIVE_LIMIT_ABS) return DRIVE_LIMIT_ABS;
    if (value < -DRIVE_LIMIT_ABS) return -DRIVE_LIMIT_ABS;
    return value;
}

static u16 clamp_steering(u16 value)
{
    if (value < STEERING_MIN_PULSE_US) return STEERING_MIN_PULSE_US;
    if (value > STEERING_MAX_PULSE_US) return STEERING_MAX_PULSE_US;
    return value;
}

static u8 outputs_must_be_centered(const app_context_t *ctx)
{
    if (ctx->kill_switch != APP_FALSE) return APP_TRUE;
    if (ctx->app_state == APP_STATE_FINISHED) return APP_TRUE;
    if (ctx->app_state == APP_STATE_GROUND_FAULT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_SUCTION_LOCKOUT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD) return APP_TRUE;
    if (ctx->app_state == APP_STATE_HARD_FAULT) return APP_TRUE;
    return (app_safety_outputs_allowed(ctx) == APP_FALSE);
}

static u8 suction_must_be_off(const app_context_t *ctx)
{
    if (ctx->kill_switch != APP_FALSE) return APP_TRUE;
    if (ctx->app_state == APP_STATE_FINISHED) return APP_TRUE;
    if (ctx->app_state == APP_STATE_GROUND_FAULT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_SUCTION_LOCKOUT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_HARD_FAULT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD) return APP_FALSE;
    return (app_safety_outputs_allowed(ctx) == APP_FALSE);
}

void app_output_arbitrate(app_context_t *ctx)
{
    if (ctx == 0) {
        return;
    }

    if (ctx->kill_switch != APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_KILL_SWITCH);
        ctx->app_state = APP_STATE_HARD_FAULT;
    }

    if (outputs_must_be_centered(ctx) != APP_FALSE) {
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_pulse_us = STEERING_SAFE_CENTER_US;
        ctx->steering_left_pulse_us = STEERING_SAFE_CENTER_US;
        ctx->steering_right_pulse_us = STEERING_SAFE_CENTER_US;
        if (suction_must_be_off(ctx) != APP_FALSE) {
            ctx->suction_cmd.mode = SUCTION_OFF;
            ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
            ctx->suction_cmd.armed = APP_FALSE;
        }
        return;
    }

    ctx->drive_command_native = clamp_drive(ctx->drive_command_native);
    ctx->left_drive_command_native = ctx->drive_command_native;
    ctx->right_drive_command_native = ctx->drive_command_native;
    ctx->steering_pulse_us = clamp_steering(ctx->steering_pulse_us);
    ctx->steering_left_pulse_us = ctx->steering_pulse_us;
    ctx->steering_right_pulse_us = ctx->steering_pulse_us;
}

#include "app_output_arbitration.h"
#include "app_config.h"
#include "app_safety.h"

static s16 clamp_drive(s16 value)
{
    if (value > DRIVE_LIMIT_ABS) return DRIVE_LIMIT_ABS;
    if (value < -DRIVE_LIMIT_ABS) return -DRIVE_LIMIT_ABS;
    return value;
}

static u8 outputs_must_be_safe(const app_context_t *ctx)
{
    if (ctx->kill_switch != APP_FALSE) return APP_TRUE;
    if (ctx->app_state == APP_STATE_FINISHED) return APP_TRUE;
    if (ctx->app_state == APP_STATE_GROUND_FAULT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_SUCTION_LOCKOUT) return APP_TRUE;
    if (ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD) return APP_TRUE;
    if (ctx->app_state == APP_STATE_HARD_FAULT) return APP_TRUE;
    return (app_safety_outputs_allowed(ctx) == APP_FALSE);
}

static void fan_off(app_context_t *ctx)
{
    ctx->fan_cmd.state = FAN_ESC_OFF;
    ctx->fan_cmd.request_us = FAN_ESC_MIN_US;
    ctx->fan_cmd.output_us = 0u;
    ctx->fan_cmd.physical_enabled = APP_FALSE;
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

    if (outputs_must_be_safe(ctx) != APP_FALSE) {
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->turn_delta_mm_s = 0;
        if (ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD) {
            ctx->fan_cmd.state = FAN_ESC_FAILSAFE_HOLD;
            ctx->fan_cmd.request_us = FAN_HOLD_US;
            ctx->fan_cmd.output_us = 0u;
            ctx->fan_cmd.physical_enabled = APP_FALSE;
        } else {
            fan_off(ctx);
        }
        return;
    }

    ctx->left_drive_command_native = clamp_drive(ctx->left_drive_command_native);
    ctx->right_drive_command_native = clamp_drive(ctx->right_drive_command_native);
    ctx->drive_command_native = (s16)(((s32)ctx->left_drive_command_native +
                                       (s32)ctx->right_drive_command_native) / 2l);
}

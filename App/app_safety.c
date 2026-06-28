#include "app_safety.h"
#include "app_config.h"

safety_profile_t app_safety_select_profile(const app_context_t *ctx)
{
    if (ctx == 0) {
        return SAFETY_PROFILE_HARD_POWER_OR_THERMAL_FAULT;
    }
    if ((ctx->faults & FAULT_HARD_POWER) != 0u) {
        return SAFETY_PROFILE_HARD_POWER_OR_THERMAL_FAULT;
    }
    if (ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION ||
        ctx->app_state == APP_STATE_TRANSITION_UP ||
        ctx->app_state == APP_STATE_WALL_TRACK ||
        ctx->app_state == APP_STATE_TRANSITION_DOWN ||
        ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD ||
        ctx->surface_state == SURFACE_WALL) {
        if (ctx->faults != FAULT_NONE) {
            return SAFETY_PROFILE_WALL_OR_UNKNOWN_FAULT;
        }
    }
    if (ctx->faults != FAULT_NONE) {
        return SAFETY_PROFILE_GROUND_FAULT;
    }
    return SAFETY_PROFILE_NONE;
}

void app_safety_apply_profile(app_context_t *ctx, safety_profile_t profile)
{
    if (ctx == 0) {
        return;
    }

    if (profile == SAFETY_PROFILE_GROUND_FAULT) {
        /* GROUND_FAULT: drive zero, steering safe, suction off or cooldown. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_pulse_us = STEERING_SAFE_CENTER_US;
        ctx->suction_cmd.mode = SUCTION_OFF;
        ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
        ctx->app_state = APP_STATE_GROUND_FAULT;
    } else if (profile == SAFETY_PROFILE_WALL_OR_UNKNOWN_FAULT) {
        /* WALL_OR_UNKNOWN_FAULT: drive zero, steering safe, request emergency hold. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_pulse_us = STEERING_SAFE_CENTER_US;
        ctx->suction_cmd.mode = SUCTION_EMERGENCY_HOLD;
        ctx->suction_cmd.command_native = SUCTION_EMERGENCY_HOLD_NATIVE;
        ctx->app_state = APP_STATE_WALL_FAILSAFE_HOLD;
    } else if (profile == SAFETY_PROFILE_HARD_POWER_OR_THERMAL_FAULT) {
        /* HARD_POWER_OR_THERMAL_FAULT: protect electrical system first. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_pulse_us = STEERING_SAFE_CENTER_US;
        ctx->suction_cmd.mode = SUCTION_OFF;
        ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
        ctx->app_state = APP_STATE_HARD_FAULT;
    }
}

u8 app_safety_outputs_allowed(const app_context_t *ctx)
{
    if (ctx == 0) {
        return APP_FALSE;
    }
    if (ctx->manual_arm == APP_FALSE) {
        return APP_FALSE;
    }
    if (ctx->app_state == APP_STATE_ARMED_GROUND ||
        ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION ||
        ctx->app_state == APP_STATE_TRANSITION_UP ||
        ctx->app_state == APP_STATE_WALL_TRACK) {
        return APP_TRUE;
    }
    return APP_FALSE;
}

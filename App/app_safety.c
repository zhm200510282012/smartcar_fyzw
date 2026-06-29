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
    if (ctx->app_state == APP_STATE_SUCTION_LOCKOUT) {
        return SAFETY_PROFILE_SUCTION_LOCKOUT;
    }
    if (ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_TRANSITION_CANDIDATE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION ||
        ctx->app_state == APP_STATE_TRANSITION_UP ||
        ctx->app_state == APP_STATE_WALL_TRACK ||
        ctx->app_state == APP_STATE_CYLINDER_TRACK ||
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
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_left_pulse_us = STEERING_LEFT_CENTER_US;
        ctx->steering_right_pulse_us = STEERING_RIGHT_CENTER_US;
        ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                        (u32)ctx->steering_right_pulse_us) / 2ul);
        ctx->suction_cmd.mode = SUCTION_OFF;
        ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
        ctx->app_state = APP_STATE_GROUND_FAULT;
    } else if (profile == SAFETY_PROFILE_WALL_OR_UNKNOWN_FAULT) {
        /* WALL_OR_UNKNOWN_FAULT: drive zero, steering safe, request emergency hold. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_left_pulse_us = STEERING_LEFT_CENTER_US;
        ctx->steering_right_pulse_us = STEERING_RIGHT_CENTER_US;
        ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                        (u32)ctx->steering_right_pulse_us) / 2ul);
        ctx->suction_cmd.mode = SUCTION_EMERGENCY_HOLD;
        ctx->suction_cmd.command_native = SUCTION_EMERGENCY_HOLD_NATIVE;
        ctx->app_state = APP_STATE_WALL_FAILSAFE_HOLD;
    } else if (profile == SAFETY_PROFILE_SUCTION_LOCKOUT) {
        /* SUCTION_LOCKOUT: expected refusal before any wall-related state. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_left_pulse_us = STEERING_LEFT_CENTER_US;
        ctx->steering_right_pulse_us = STEERING_RIGHT_CENTER_US;
        ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                        (u32)ctx->steering_right_pulse_us) / 2ul);
        ctx->suction_cmd.mode = SUCTION_OFF;
        ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
        ctx->suction_cmd.armed = APP_FALSE;
        ctx->suction_cmd.hw_verified = APP_FALSE;
        ctx->suction_cmd.feedback_valid = APP_FALSE;
        ctx->suction_cmd.fault_code = (u8)(ctx->suction_cmd.fault_code | FAULT_SUCTION_LOCKOUT);
        ctx->app_state = APP_STATE_SUCTION_LOCKOUT;
    } else if (profile == SAFETY_PROFILE_HARD_POWER_OR_THERMAL_FAULT) {
        /* HARD_POWER_OR_THERMAL_FAULT: protect electrical system first. */
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->steering_offset_us = 0;
        ctx->steering_left_pulse_us = STEERING_LEFT_CENTER_US;
        ctx->steering_right_pulse_us = STEERING_RIGHT_CENTER_US;
        ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                        (u32)ctx->steering_right_pulse_us) / 2ul);
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
        ctx->app_state == APP_STATE_GROUND_TRACK ||
        ctx->app_state == APP_STATE_TRANSITION_CANDIDATE ||
        ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION ||
        ctx->app_state == APP_STATE_TRANSITION_UP ||
        ctx->app_state == APP_STATE_WALL_TRACK ||
        ctx->app_state == APP_STATE_CYLINDER_TRACK ||
        ctx->app_state == APP_STATE_SEESAW_PASS) {
        return APP_TRUE;
    }
    return APP_FALSE;
}

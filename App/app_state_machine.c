#include "app_state_machine.h"
#include "app_build_profile.h"
#include "app_config.h"

static u8 sensors_ready(const app_context_t *ctx)
{
    return (ctx->health.emag_ok && ctx->health.imu_fresh && ctx->health.encoder_ok && ctx->health.control_period_ok);
}

static void enter_state(app_context_t *ctx, app_state_t state)
{
    ctx->app_state = state;
    ctx->state_elapsed_ms = 0u;
}

static u8 ground_observed(const app_context_t *ctx)
{
    return (ctx->surface_state == SURFACE_GROUND);
}

static u8 transition_up_observed(const app_context_t *ctx)
{
    return (ctx->surface_state == SURFACE_TRANSITION_UP || ctx->surface_state == SURFACE_WALL);
}

static u8 transition_candidate_detected(const app_context_t *ctx)
{
    return (ctx->transition_candidate == APP_TRUE);
}

static u8 wall_observed(const app_context_t *ctx)
{
    return (ctx->surface_state == SURFACE_WALL);
}

static u8 cylinder_observed(const app_context_t *ctx)
{
    return (ctx->surface_state == SURFACE_CYLINDER);
}

static u8 transition_down_observed(const app_context_t *ctx)
{
    return (ctx->surface_state == SURFACE_TRANSITION_DOWN || ctx->surface_state == SURFACE_GROUND);
}

void app_state_machine_init(app_context_t *ctx)
{
    if (ctx == 0) {
        return;
    }
    ctx->app_state = APP_STATE_BOOT;
    ctx->surface_state = SURFACE_UNKNOWN;
    ctx->faults = FAULT_NONE;
    ctx->state_elapsed_ms = 0u;
    ctx->manual_arm = APP_FALSE;
    ctx->manual_suction_authorize = APP_FALSE;
    ctx->transition_candidate = APP_FALSE;
    ctx->kill_switch = APP_FALSE;
    ctx->test_mode = APP_TEST_MODE;
    ctx->track_mode = TRACK_MODE_STRAIGHT;
    ctx->course_segment = TRACK_COURSE_START;
    ctx->adhesion_risk = 1000u;
    ctx->speed_limit_mm_s = DRIVE_SAFE_ZERO;
    ctx->target_speed_mm_s = DRIVE_SAFE_ZERO;
    ctx->left_speed_target_mm_s = DRIVE_SAFE_ZERO;
    ctx->right_speed_target_mm_s = DRIVE_SAFE_ZERO;
    ctx->drive_command_native = DRIVE_SAFE_ZERO;
    ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
    ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
    ctx->line_error_filtered = 0;
    ctx->line_error_rate = 0;
    ctx->fuzzy_kp = FUZZY_STRAIGHT_BASE_KP;
    ctx->fuzzy_ki = FUZZY_STRAIGHT_BASE_KI;
    ctx->fuzzy_kd = FUZZY_STRAIGHT_BASE_KD;
    ctx->turn_delta_mm_s = 0;
    ctx->steering_offset_us = 0;
    ctx->steering_left_pulse_us = STEERING_LEFT_CENTER_US;
    ctx->steering_right_pulse_us = STEERING_RIGHT_CENTER_US;
    ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                    (u32)ctx->steering_right_pulse_us) / 2ul);
    ctx->suction_cmd.mode = SUCTION_OFF;
    ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
    ctx->suction_cmd.armed = APP_FALSE;
    ctx->suction_cmd.hw_verified = SUCTION_HW_VERIFIED;
    ctx->suction_cmd.feedback_valid = APP_FALSE;
    ctx->suction_cmd.fault_code = FAULT_NONE;
    ctx->fan_cmd.state = FAN_ESC_OFF;
    ctx->fan_cmd.request_us = FAN_ESC_MIN_US;
    ctx->fan_cmd.output_us = 0u;
    ctx->fan_cmd.mapped = APP_FALSE;
    ctx->fan_cmd.physical_enabled = APP_FALSE;
    ctx->wall_state = TRACK_WALL_GROUND_TRACK;
    ctx->emag.channel_count = 0u;
    ctx->emag.line_quality = 0u;
    ctx->emag.signal_quality = 0u;
    ctx->emag.line_lost = APP_TRUE;
    ctx->emag.valid = APP_FALSE;
    ctx->attitude.fresh = APP_FALSE;
    ctx->attitude.id_ok = APP_FALSE;
    ctx->attitude.pitch_rate_cdeg_s = 0;
    ctx->encoder.valid = APP_FALSE;
    ctx->health.emag_ok = APP_FALSE;
    ctx->health.imu_fresh = APP_FALSE;
    ctx->health.encoder_ok = APP_FALSE;
    ctx->health.power_ok = APP_FALSE;
    ctx->health.control_period_ok = APP_FALSE;
    ctx->health.suction_feedback_ok = APP_FALSE;
}

void app_state_machine_step(app_context_t *ctx, u16 dt_ms)
{
    if (ctx == 0) {
        return;
    }

    ctx->state_elapsed_ms = (u16)(ctx->state_elapsed_ms + dt_ms);

    switch (ctx->app_state) {
    case APP_STATE_BOOT:
        enter_state(ctx, APP_STATE_SELF_CHECK);
        break;
    case APP_STATE_SELF_CHECK:
        if (ctx->health.power_ok) {
            enter_state(ctx, APP_STATE_SENSOR_CALIBRATION);
        }
        break;
    case APP_STATE_SENSOR_CALIBRATION:
        if (sensors_ready(ctx) && ground_observed(ctx)) {
            enter_state(ctx, APP_STATE_SAFE_GROUND_READY);
        }
        break;
    case APP_STATE_SAFE_GROUND_READY:
        if (ctx->manual_arm && sensors_ready(ctx)) {
            enter_state(ctx, APP_STATE_ARMED_GROUND);
        }
        break;
    case APP_STATE_ARMED_GROUND:
        if (sensors_ready(ctx) && ground_observed(ctx)) {
            enter_state(ctx, APP_STATE_GROUND_TRACK);
        }
        break;
    case APP_STATE_GROUND_TRACK:
        if (ctx->manual_suction_authorize && sensors_ready(ctx) && transition_candidate_detected(ctx)) {
            if (APP_WALL_STATE_CAPABLE != 0) {
                enter_state(ctx, APP_STATE_TRANSITION_CANDIDATE);
            } else {
                ctx->faults = (fault_code_t)(ctx->faults | FAULT_SUCTION_LOCKOUT);
                enter_state(ctx, APP_STATE_SUCTION_LOCKOUT);
            }
        }
        break;
    case APP_STATE_TRANSITION_CANDIDATE:
        if (!sensors_ready(ctx) || !transition_candidate_detected(ctx)) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else {
            enter_state(ctx, APP_STATE_SUCTION_PRECHARGE);
        }
        break;
    case APP_STATE_SUCTION_PRECHARGE:
        if (!sensors_ready(ctx) || !transition_candidate_detected(ctx) || ctx->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (ctx->state_elapsed_ms >= PRECHARGE_MIN_TIME_MS) {
            enter_state(ctx, APP_STATE_APPROACH_TRANSITION);
        }
        break;
    case APP_STATE_APPROACH_TRANSITION:
        if (!sensors_ready(ctx) ||
            (!transition_candidate_detected(ctx) && !transition_up_observed(ctx)) ||
            ctx->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (transition_up_observed(ctx)) {
            enter_state(ctx, APP_STATE_TRANSITION_UP);
        }
        break;
    case APP_STATE_TRANSITION_UP:
        if (!sensors_ready(ctx) || ctx->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (wall_observed(ctx)) {
            enter_state(ctx, APP_STATE_WALL_TRACK);
        }
        break;
    case APP_STATE_WALL_TRACK:
        if (!sensors_ready(ctx)) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (cylinder_observed(ctx)) {
            enter_state(ctx, APP_STATE_CYLINDER_TRACK);
        } else if (transition_down_observed(ctx)) {
            enter_state(ctx, APP_STATE_TRANSITION_DOWN);
        }
        break;
    case APP_STATE_CYLINDER_TRACK:
        if (!sensors_ready(ctx)) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (wall_observed(ctx)) {
            enter_state(ctx, APP_STATE_WALL_TRACK);
        } else if (transition_down_observed(ctx)) {
            enter_state(ctx, APP_STATE_TRANSITION_DOWN);
        }
        break;
    case APP_STATE_TRANSITION_DOWN:
        if (!sensors_ready(ctx) || ctx->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (ground_observed(ctx)) {
            enter_state(ctx, APP_STATE_GROUND_RECOVERY);
        }
        break;
    case APP_STATE_GROUND_RECOVERY:
        if (!sensors_ready(ctx) || transition_up_observed(ctx) || cylinder_observed(ctx)) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        } else if (ctx->state_elapsed_ms >= GROUND_CONFIRM_TIME_MS && ground_observed(ctx)) {
            enter_state(ctx, APP_STATE_GROUND_TRACK);
        }
        break;
    case APP_STATE_SEESAW_PASS:
        if (ground_observed(ctx)) {
            enter_state(ctx, APP_STATE_GROUND_TRACK);
        }
        break;
    case APP_STATE_SUCTION_LOCKOUT:
    case APP_STATE_GROUND_FAULT:
    case APP_STATE_WALL_FAILSAFE_HOLD:
    case APP_STATE_HARD_FAULT:
    case APP_STATE_FINISHED:
        break;
    default:
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_HARD_POWER);
        enter_state(ctx, APP_STATE_HARD_FAULT);
        break;
    }
}

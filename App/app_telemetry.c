#include "app_telemetry.h"
#include "app_config.h"

telemetry_frame_t app_telemetry_make_frame(const app_context_t *ctx, u32 now_ms)
{
    telemetry_frame_t t;
    t.timestamp_ms = now_ms;
    if (ctx == 0) {
        t.app_state = APP_STATE_HARD_FAULT;
        t.surface_state = SURFACE_UNKNOWN;
        t.track_mode = TRACK_MODE_RECOVERY;
        t.faults = FAULT_HARD_POWER;
        t.line_error = 0;
        t.line_error_filtered = 0;
        t.error_rate = 0;
        t.signal_quality = 0u;
        t.drive_command_native = DRIVE_SAFE_ZERO;
        t.steering_offset_us = 0;
        t.steering_pulse_us = STEERING_SAFE_CENTER_US;
        t.steering_left_pulse_us = STEERING_SAFE_CENTER_US;
        t.steering_right_pulse_us = STEERING_SAFE_CENTER_US;
        t.fuzzy_kp = 0;
        t.fuzzy_ki = 0;
        t.fuzzy_kd = 0;
        t.left_speed_target_mm_s = 0;
        t.right_speed_target_mm_s = 0;
        t.left_speed_measured_mm_s = 0;
        t.right_speed_measured_mm_s = 0;
        t.suction_mode = SUCTION_OFF;
        t.suction_request_native = 0u;
        t.adhesion_risk = 1000u;
        return t;
    }
    t.app_state = ctx->app_state;
    t.surface_state = ctx->surface_state;
    t.track_mode = ctx->track_mode;
    t.faults = ctx->faults;
    t.line_error = ctx->emag.line_error;
    t.line_error_filtered = ctx->line_error_filtered;
    t.error_rate = ctx->line_error_rate;
    t.signal_quality = ctx->emag.signal_quality;
    t.drive_command_native = ctx->drive_command_native;
    t.steering_offset_us = ctx->steering_offset_us;
    t.steering_pulse_us = ctx->steering_pulse_us;
    t.steering_left_pulse_us = ctx->steering_left_pulse_us;
    t.steering_right_pulse_us = ctx->steering_right_pulse_us;
    t.fuzzy_kp = ctx->fuzzy_kp;
    t.fuzzy_ki = ctx->fuzzy_ki;
    t.fuzzy_kd = ctx->fuzzy_kd;
    t.left_speed_target_mm_s = ctx->left_speed_target_mm_s;
    t.right_speed_target_mm_s = ctx->right_speed_target_mm_s;
    t.left_speed_measured_mm_s = ctx->encoder.left_speed_mm_s;
    t.right_speed_measured_mm_s = ctx->encoder.right_speed_mm_s;
    t.suction_mode = ctx->suction_cmd.mode;
    t.suction_request_native = ctx->suction_cmd.command_native;
    t.adhesion_risk = ctx->adhesion_risk;
    return t;
}

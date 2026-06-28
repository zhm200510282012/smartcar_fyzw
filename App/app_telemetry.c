#include "app_telemetry.h"
#include "app_config.h"

telemetry_frame_t app_telemetry_make_frame(const app_context_t *ctx, u32 now_ms)
{
    telemetry_frame_t t;
    t.timestamp_ms = now_ms;
    if (ctx == 0) {
        t.app_state = APP_STATE_HARD_FAULT;
        t.surface_state = SURFACE_UNKNOWN;
        t.faults = FAULT_HARD_POWER;
        t.line_error = 0;
        t.signal_quality = 0u;
        t.drive_command_native = DRIVE_SAFE_ZERO;
        t.steering_offset_us = 0;
        t.steering_pulse_us = STEERING_SAFE_CENTER_US;
        t.suction_mode = SUCTION_OFF;
        t.suction_request_native = 0u;
        t.adhesion_risk = 1000u;
        return t;
    }
    t.app_state = ctx->app_state;
    t.surface_state = ctx->surface_state;
    t.faults = ctx->faults;
    t.line_error = ctx->emag.line_error;
    t.signal_quality = ctx->emag.signal_quality;
    t.drive_command_native = ctx->drive_command_native;
    t.steering_offset_us = ctx->steering_offset_us;
    t.steering_pulse_us = ctx->steering_pulse_us;
    t.suction_mode = ctx->suction_cmd.mode;
    t.suction_request_native = ctx->suction_cmd.command_native;
    t.adhesion_risk = ctx->adhesion_risk;
    return t;
}

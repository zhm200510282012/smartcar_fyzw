#include "ctrl_vehicle.h"
#include "ctrl_signal.h"

void ctrl_vehicle_update(app_context_t *ctx)
{
    s16 pulse;
    if (ctx == 0) return;
    if (ctx->speed_limit_mm_s <= 0) {
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
    }
    ctx->drive_command_native = ctrl_signal_clamp_s16(ctx->drive_command_native, -DRIVE_LIMIT_ABS, DRIVE_LIMIT_ABS);
    pulse = (s16)STEERING_CENTER_US + ctx->steering_offset_us;
    if (pulse < (s16)STEERING_MIN_PULSE_US) pulse = (s16)STEERING_MIN_PULSE_US;
    if (pulse > (s16)STEERING_MAX_PULSE_US) pulse = (s16)STEERING_MAX_PULSE_US;
    ctx->steering_pulse_us = (u16)pulse;
}

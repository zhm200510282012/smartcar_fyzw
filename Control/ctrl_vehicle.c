/* historical / inactive / not used by runtime: final car uses differential drive, not servo pulse arbitration. */
#include "ctrl_vehicle.h"
#include "ctrl_signal.h"

static u16 clamp_u16_to_range(s16 value, u16 min_value, u16 max_value)
{
    if (value < (s16)min_value) return min_value;
    if (value > (s16)max_value) return max_value;
    return (u16)value;
}

void ctrl_vehicle_update(app_context_t *ctx)
{
    s16 left_pulse;
    s16 right_pulse;
    if (ctx == 0) return;
    if (ctx->speed_limit_mm_s <= 0) {
        ctx->drive_command_native = DRIVE_SAFE_ZERO;
        ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
        ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
    }
    ctx->left_drive_command_native = ctrl_signal_clamp_s16(ctx->left_drive_command_native, -DRIVE_LIMIT_ABS, DRIVE_LIMIT_ABS);
    ctx->right_drive_command_native = ctrl_signal_clamp_s16(ctx->right_drive_command_native, -DRIVE_LIMIT_ABS, DRIVE_LIMIT_ABS);
    ctx->drive_command_native = (s16)(((s32)ctx->left_drive_command_native + (s32)ctx->right_drive_command_native) / 2l);

    left_pulse = (s16)STEERING_LEFT_CENTER_US +
                 (s16)(STEERING_LEFT_SIGN * ctx->steering_offset_us);
    right_pulse = (s16)STEERING_RIGHT_CENTER_US +
                  (s16)(STEERING_RIGHT_SIGN * ctx->steering_offset_us);
    ctx->steering_left_pulse_us = clamp_u16_to_range(left_pulse,
                                                     STEERING_LEFT_MIN_US,
                                                     STEERING_LEFT_MAX_US);
    ctx->steering_right_pulse_us = clamp_u16_to_range(right_pulse,
                                                      STEERING_RIGHT_MIN_US,
                                                      STEERING_RIGHT_MAX_US);
    ctx->steering_pulse_us = (u16)(((u32)ctx->steering_left_pulse_us +
                                    (u32)ctx->steering_right_pulse_us) / 2ul);
}

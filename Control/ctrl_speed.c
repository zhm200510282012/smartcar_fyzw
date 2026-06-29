/*
 * 模块职责：左右轮独立速度 PI。
 * 输入：左右目标速度、编码器测得速度，单位 mm/s。
 * 输出：左右 native PWM/方向命令；encoder invalid 时清积分并归零。
 * 说明：积分限幅防止长时间饱和，输出限幅和加速度限幅用于离地低功率起步。
 */
#include "ctrl_speed.h"

static s32 clamp_s32(s32 value, s32 min_value, s32 max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static s16 clamp_s16_local(s16 value, s16 min_value, s16 max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static s16 apply_accel_limit(s16 desired, s16 previous)
{
    s16 delta;
    delta = (s16)(desired - previous);
    if (delta > SPEED_ACCEL_LIMIT) {
        return (s16)(previous + SPEED_ACCEL_LIMIT);
    }
    if (delta < (s16)-SPEED_ACCEL_LIMIT) {
        return (s16)(previous - SPEED_ACCEL_LIMIT);
    }
    return desired;
}

void ctrl_speed_pi_init(speed_pi_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->integral = 0l;
    state->previous_output = DRIVE_SAFE_ZERO;
}

void ctrl_speed_pi_reset(speed_pi_state_t *state)
{
    ctrl_speed_pi_init(state);
}

s16 ctrl_speed_pi_update(speed_pi_state_t *state,
                         s16 target_mm_s,
                         s16 measured_mm_s,
                         u8 encoder_valid)
{
    s16 error;
    s32 desired;
    s16 limited;

    if (state == 0 || encoder_valid == APP_FALSE) {
        if (state != 0) {
            ctrl_speed_pi_reset(state);
        }
        return DRIVE_SAFE_ZERO;
    }

    error = (s16)(target_mm_s - measured_mm_s);
    state->integral = clamp_s32(state->integral + (s32)error,
                                (s32)-SPEED_INTEGRAL_LIMIT,
                                (s32)SPEED_INTEGRAL_LIMIT);

    desired = (((s32)SPEED_KP * (s32)error) +
               ((s32)SPEED_KI * state->integral)) / 100l;
    desired = clamp_s32(desired, (s32)-SPEED_OUTPUT_LIMIT, (s32)SPEED_OUTPUT_LIMIT);
    limited = apply_accel_limit((s16)desired, state->previous_output);
    limited = clamp_s16_local(limited, (s16)-SPEED_OUTPUT_LIMIT, SPEED_OUTPUT_LIMIT);
    state->previous_output = limited;
    return limited;
}

speed_pi_output_t ctrl_speed_update_pair(speed_pi_state_t *left_state,
                                         speed_pi_state_t *right_state,
                                         s16 left_target_mm_s,
                                         s16 right_target_mm_s,
                                         encoder_sample_t encoder)
{
    speed_pi_output_t output;
    output.left_native = ctrl_speed_pi_update(left_state,
                                             left_target_mm_s,
                                             encoder.left_speed_mm_s,
                                             encoder.valid);
    output.right_native = ctrl_speed_pi_update(right_state,
                                              right_target_mm_s,
                                              encoder.right_speed_mm_s,
                                              encoder.valid);
    output.average_native = (s16)(((s32)output.left_native + (s32)output.right_native) / 2l);
    return output;
}

s16 ctrl_speed_command_native(s16 target_mm_s, encoder_sample_t encoder)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, target_mm_s, target_mm_s, encoder);
    return output.average_native;
}

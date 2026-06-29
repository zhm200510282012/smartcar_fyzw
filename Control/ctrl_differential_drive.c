/*
 * 模块职责：把基础速度和转向差速量混成左右轮目标速度。
 * 输入：base_speed_mm_s 与 turn_delta_mm_s。
 * 输出：left/right_target_mm_s；公式为 left=base+sign*turn，right=base-sign*turn。
 * 关系：本模块不直接写 PWM，后级 ctrl_speed 对左右轮分别做 PI。
 */
#include "ctrl_differential_drive.h"
#include "ctrl_signal.h"

static s16 clamp_speed_target(s32 value)
{
    if (value < 0l) return 0;
    if (value > (s32)DIFF_TARGET_SPEED_LIMIT_MM_S) return DIFF_TARGET_SPEED_LIMIT_MM_S;
    return (s16)value;
}

differential_drive_output_t ctrl_differential_drive_mix(s16 base_speed_mm_s,
                                                        s16 turn_delta_mm_s,
                                                        u8 outputs_allowed)
{
    differential_drive_output_t out;
    s16 limited_turn;
    s32 left;
    s32 right;

    out.left_target_mm_s = 0;
    out.right_target_mm_s = 0;
    out.valid = APP_FALSE;

    if (outputs_allowed == APP_FALSE || base_speed_mm_s <= 0) {
        return out;
    }

    limited_turn = ctrl_signal_clamp_s16(turn_delta_mm_s,
                                         (s16)-DIFF_TURN_DELTA_LIMIT_MM_S,
                                         DIFF_TURN_DELTA_LIMIT_MM_S);
    left = (s32)base_speed_mm_s + ((s32)DIFF_TURN_SIGN * (s32)limited_turn);
    right = (s32)base_speed_mm_s - ((s32)DIFF_TURN_SIGN * (s32)limited_turn);

    out.left_target_mm_s = clamp_speed_target(left);
    out.right_target_mm_s = clamp_speed_target(right);
    out.valid = APP_TRUE;
    return out;
}

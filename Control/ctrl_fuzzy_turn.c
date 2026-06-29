/*
 * 模块职责：地面/墙面统一转向控制，输出差速转向量。
 * 输入：line_error_filtered、error_rate、track_mode 和 app_state。
 * 输出：turn_delta_mm_s；FUZZY_ENABLE=0 时使用基础 PD，=1 时才启用模糊自整定。
 * 关系：本模块只修正左右速度目标，不直接控制 PWM、风机或历史舵机。
 */
#include "ctrl_fuzzy_turn.h"
#include "ctrl_signal.h"

static fuzzy_pid_base_t base_from_mode_params(const track_mode_params_t *params)
{
    fuzzy_pid_base_t base;
    if (params == 0) {
        base.base_kp = FUZZY_STRAIGHT_BASE_KP;
        base.base_ki = FUZZY_STRAIGHT_BASE_KI;
        base.base_kd = FUZZY_STRAIGHT_BASE_KD;
        base.kp_adjust_limit = FUZZY_STRAIGHT_KP_LIMIT;
        base.ki_adjust_limit = FUZZY_STRAIGHT_KI_LIMIT;
        base.kd_adjust_limit = FUZZY_STRAIGHT_KD_LIMIT;
        return base;
    }
    base.base_kp = params->base_kp;
    base.base_ki = params->base_ki;
    base.base_kd = params->base_kd;
    base.kp_adjust_limit = params->kp_adjust_limit;
    base.ki_adjust_limit = params->ki_adjust_limit;
    base.kd_adjust_limit = params->kd_adjust_limit;
    return base;
}

static s16 clamp_turn(s16 value, s16 limit)
{
    if (limit < 0) {
        limit = (s16)-limit;
    }
    return ctrl_signal_clamp_s16(value, (s16)-limit, limit);
}

static s16 apply_rate_limit(s16 desired, s16 previous, s16 rate_limit)
{
    s16 delta;
    if (rate_limit <= 0) {
        return desired;
    }
    delta = (s16)(desired - previous);
    if (delta > rate_limit) {
        return (s16)(previous + rate_limit);
    }
    if (delta < (s16)-rate_limit) {
        return (s16)(previous - rate_limit);
    }
    return desired;
}

void ctrl_fuzzy_turn_init(ctrl_fuzzy_turn_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->last_mode = TRACK_MODE_STRAIGHT;
    state->integral = 0;
    state->previous_error = 0;
    state->previous_output_mm_s = 0;
    state->fuzzy_elapsed_ms = FUZZY_UPDATE_PERIOD_MS;
    state->gain_valid = APP_FALSE;
    state->gain.kp = FUZZY_STRAIGHT_BASE_KP;
    state->gain.ki = FUZZY_STRAIGHT_BASE_KI;
    state->gain.kd = FUZZY_STRAIGHT_BASE_KD;
    state->adjust.dkp = 0;
    state->adjust.dki = 0;
    state->adjust.dkd = 0;
}

void ctrl_fuzzy_turn_reset(ctrl_fuzzy_turn_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->integral = 0;
    state->previous_error = 0;
    state->previous_output_mm_s = 0;
    state->fuzzy_elapsed_ms = FUZZY_UPDATE_PERIOD_MS;
    state->gain_valid = APP_FALSE;
}

u8 ctrl_fuzzy_turn_needs_reset(app_state_t app_state,
                               track_mode_t mode,
                               u16 signal_quality,
                               u8 outputs_allowed)
{
    if (outputs_allowed == APP_FALSE) return APP_TRUE;
    if (signal_quality < TRACK_LINE_LOST_QUALITY_MIN) return APP_TRUE;
    if (mode == TRACK_MODE_LINE_LOST) return APP_TRUE;
    if (app_state == APP_STATE_FINISHED) return APP_TRUE;
    if (app_state == APP_STATE_GROUND_FAULT) return APP_TRUE;
    if (app_state == APP_STATE_SUCTION_LOCKOUT) return APP_TRUE;
    if (app_state == APP_STATE_WALL_FAILSAFE_HOLD) return APP_TRUE;
    if (app_state == APP_STATE_HARD_FAULT) return APP_TRUE;
    return APP_FALSE;
}

s16 ctrl_fuzzy_turn_update(ctrl_fuzzy_turn_state_t *state,
                           const ctrl_fuzzy_turn_input_t *input)
{
    const track_mode_params_t *params;
    fuzzy_pid_base_t base;
#if FUZZY_ENABLE != 0
    s16 e_norm;
    s16 de_norm;
#endif
    s32 output;
    s32 integral;
    s16 limited;

    if (state == 0 || input == 0) {
        return 0;
    }

    params = track_strategy_mode_params(input->mode);
    if (ctrl_fuzzy_turn_needs_reset(input->app_state,
                                    input->mode,
                                    input->signal_quality,
                                    input->outputs_allowed) != APP_FALSE) {
        ctrl_fuzzy_turn_reset(state);
        state->last_mode = input->mode;
        state->gain.kp = params->base_kp;
        state->gain.ki = params->base_ki;
        state->gain.kd = params->base_kd;
        return 0;
    }

    if (state->last_mode != input->mode) {
        state->integral = 0;
        state->previous_error = input->line_error_filtered;
        state->previous_output_mm_s = 0;
        state->fuzzy_elapsed_ms = FUZZY_UPDATE_PERIOD_MS;
        state->gain_valid = APP_FALSE;
        state->last_mode = input->mode;
    }

    state->fuzzy_elapsed_ms = (u16)(state->fuzzy_elapsed_ms + input->dt_ms);
    if (state->gain_valid == APP_FALSE ||
        state->fuzzy_elapsed_ms >= FUZZY_UPDATE_PERIOD_MS) {
        base = base_from_mode_params(params);
#if FUZZY_ENABLE == 0
        state->gain.kp = base.base_kp;
        state->gain.ki = base.base_ki;
        state->gain.kd = base.base_kd;
        state->adjust.dkp = 0;
        state->adjust.dki = 0;
        state->adjust.dkd = 0;
#else
        e_norm = ctrl_fuzzy_pid_normalize(input->line_error_filtered, FUZZY_E_SCALE);
        de_norm = ctrl_fuzzy_pid_normalize(input->error_rate, FUZZY_DE_SCALE);
        ctrl_fuzzy_pid_eval(e_norm, de_norm, &base, &state->gain, &state->adjust);
#endif
        state->fuzzy_elapsed_ms = 0u;
        state->gain_valid = APP_TRUE;
    }

    if (state->gain.ki == 0) {
        state->integral = 0;
    } else {
        integral = (s32)state->integral + (s32)input->line_error_filtered;
        if (integral > 2000l) integral = 2000l;
        if (integral < -2000l) integral = -2000l;
        state->integral = (s16)integral;
    }

    output = ((s32)state->gain.kp * (s32)input->line_error_filtered) +
             ((s32)state->gain.ki * (s32)state->integral) +
             ((s32)state->gain.kd * (s32)input->error_rate);
    output = output / FUZZY_PID_OUTPUT_SCALE;
    if (output > 32767l) output = 32767l;
    if (output < -32768l) output = -32768l;

    limited = clamp_turn((s16)output, params->steering_offset_limit_us);
    limited = apply_rate_limit(limited,
                               state->previous_output_mm_s,
                               params->steering_rate_limit_us_per_tick);
    limited = clamp_turn(limited, DIFF_TURN_DELTA_LIMIT_MM_S);

    state->previous_error = input->line_error_filtered;
    state->previous_output_mm_s = limited;
    return limited;
}

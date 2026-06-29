#include <stdio.h>
#include <stdlib.h>

#include "../../App/app_config.h"
#include "../../App/app_types.h"
#include "../../Control/ctrl_fuzzy_pid.h"
#include "../../Control/ctrl_fuzzy_steering.h"
#include "../../Track/track_route_profile.h"
#include "../../Track/track_strategy.h"

static u16 abs_s16_runner(s16 value)
{
    if (value < 0) return (u16)(-value);
    return (u16)value;
}

static fuzzy_pid_base_t base_from_params(const track_mode_params_t *params)
{
    fuzzy_pid_base_t base;
    base.base_kp = params->base_kp;
    base.base_ki = params->base_ki;
    base.base_kd = params->base_kd;
    base.kp_adjust_limit = params->kp_adjust_limit;
    base.ki_adjust_limit = params->ki_adjust_limit;
    base.kd_adjust_limit = params->kd_adjust_limit;
    return base;
}

static void write_result(FILE *out,
                         const char *scenario,
                         int pass,
                         const char *notes,
                         int metric_a,
                         int metric_b,
                         int metric_c)
{
    fprintf(out,
            "%s,%s,%d,%d,%d,%s\n",
            scenario,
            pass ? "true" : "false",
            metric_a,
            metric_b,
            metric_c,
            notes);
}

static int s29_membership_boundaries(FILE *out)
{
    fuzzy_membership_t m;
    int pass;

    pass = 1;
    ctrl_fuzzy_pid_membership(-1000, &m);
    if (!(m.term_a == FUZZY_NB && m.weight_a == 1000 && m.weight_b == 0)) pass = 0;
    ctrl_fuzzy_pid_membership(-500, &m);
    if (!(m.term_b == FUZZY_NS && m.weight_b == 1000 && m.weight_a == 0)) pass = 0;
    ctrl_fuzzy_pid_membership(0, &m);
    if (!(m.term_b == FUZZY_ZO && m.weight_b == 1000 && m.weight_a == 0)) pass = 0;
    ctrl_fuzzy_pid_membership(250, &m);
    if (!(m.term_a == FUZZY_ZO && m.term_b == FUZZY_PS && m.weight_a == 500 && m.weight_b == 500)) pass = 0;
    ctrl_fuzzy_pid_membership(1000, &m);
    if (!(m.term_a == FUZZY_PB && m.weight_a == 1000 && m.weight_b == 0)) pass = 0;
    write_result(out, "S29", pass, pass ? "membership boundaries ok" : "membership boundary mismatch", m.term_a, m.weight_a, m.weight_b);
    return pass;
}

static int s30_rule_limits(FILE *out)
{
    const track_mode_params_t *params;
    fuzzy_pid_base_t base;
    fuzzy_pid_adjust_t adjust;
    fuzzy_pid_gain_t gain;
    int pass;
    int mode;
    int e_term;
    int de_term;
    int e_norm;
    int de_norm;

    pass = 1;
    for (mode = TRACK_MODE_STRAIGHT; mode <= TRACK_MODE_RECOVERY; mode++) {
        params = track_strategy_mode_params((track_mode_t)mode);
        base = base_from_params(params);
        for (e_term = 0; e_term < 5; e_term++) {
            for (de_term = 0; de_term < 5; de_term++) {
                ctrl_fuzzy_pid_rule_value((u8)e_term, (u8)de_term, &base, &adjust);
                if (abs_s16_runner(adjust.dkp) > (u16)base.kp_adjust_limit) pass = 0;
                if (abs_s16_runner(adjust.dki) > (u16)base.ki_adjust_limit) pass = 0;
                if (abs_s16_runner(adjust.dkd) > (u16)base.kd_adjust_limit) pass = 0;
            }
        }
        for (e_norm = -1000; e_norm <= 1000; e_norm += 250) {
            for (de_norm = -1000; de_norm <= 1000; de_norm += 250) {
                ctrl_fuzzy_pid_eval((s16)e_norm, (s16)de_norm, &base, &gain, &adjust);
                if (gain.kp < FUZZY_KP_MIN || gain.kp > FUZZY_KP_MAX) pass = 0;
                if (gain.ki < FUZZY_KI_MIN || gain.ki > FUZZY_KI_MAX) pass = 0;
                if (gain.kd < FUZZY_KD_MIN || gain.kd > FUZZY_KD_MAX) pass = 0;
            }
        }
    }
    write_result(out, "S30", pass, pass ? "all rule outputs limited" : "rule output outside limit", 12, 25, pass);
    return pass;
}

static int s31_straight_less_than_sharp(FILE *out)
{
    fuzzy_pid_gain_t straight_gain;
    fuzzy_pid_gain_t sharp_gain;
    fuzzy_pid_adjust_t adjust;
    fuzzy_pid_base_t straight_base;
    fuzzy_pid_base_t sharp_base;
    int pass;

    straight_base = base_from_params(track_strategy_mode_params(TRACK_MODE_STRAIGHT));
    sharp_base = base_from_params(track_strategy_mode_params(TRACK_MODE_SHARP_CURVE));
    ctrl_fuzzy_pid_eval(400, 120, &straight_base, &straight_gain, &adjust);
    ctrl_fuzzy_pid_eval(400, 120, &sharp_base, &sharp_gain, &adjust);
    pass = (straight_gain.kp < sharp_gain.kp &&
            track_strategy_target_speed_for_mode(TRACK_MODE_SHARP_CURVE) <
            track_strategy_target_speed_for_mode(TRACK_MODE_STRAIGHT));
    write_result(out, "S31", pass, pass ? "straight gain below sharp curve" : "straight/sharp gain ordering wrong", straight_gain.kp, sharp_gain.kp, track_strategy_target_speed_for_mode(TRACK_MODE_SHARP_CURVE));
    return pass;
}

static int s32_kp_not_reverse(FILE *out)
{
    fuzzy_pid_gain_t small_gain;
    fuzzy_pid_gain_t large_gain;
    fuzzy_pid_adjust_t adjust;
    fuzzy_pid_base_t base;
    int pass;

    base = base_from_params(track_strategy_mode_params(TRACK_MODE_NORMAL_CURVE));
    ctrl_fuzzy_pid_eval(100, 0, &base, &small_gain, &adjust);
    ctrl_fuzzy_pid_eval(850, 0, &base, &large_gain, &adjust);
    pass = (large_gain.kp >= small_gain.kp);
    write_result(out, "S32", pass, pass ? "kp monotonic with larger error" : "kp decreased with larger error", small_gain.kp, large_gain.kp, 0);
    return pass;
}

static int s33_kd_high_on_fast_change(FILE *out)
{
    fuzzy_pid_gain_t slow_gain;
    fuzzy_pid_gain_t fast_gain;
    fuzzy_pid_adjust_t adjust;
    fuzzy_pid_base_t base;
    int pass;

    base = base_from_params(track_strategy_mode_params(TRACK_MODE_SHARP_CURVE));
    ctrl_fuzzy_pid_eval(300, 0, &base, &slow_gain, &adjust);
    ctrl_fuzzy_pid_eval(300, 900, &base, &fast_gain, &adjust);
    pass = (fast_gain.kd >= slow_gain.kd);
    write_result(out, "S33", pass, pass ? "kd increases or stays high on fast de" : "kd dropped on fast de", slow_gain.kd, fast_gain.kd, 0);
    return pass;
}

static void fill_steering_input(ctrl_fuzzy_steering_input_t *input,
                                track_mode_t mode,
                                s16 error,
                                s16 rate,
                                u16 quality,
                                app_state_t app_state)
{
    input->mode = mode;
    input->app_state = app_state;
    input->line_error_filtered = error;
    input->error_rate = rate;
    input->signal_quality = quality;
    input->dt_ms = TASK_CONTROL_PERIOD_MS;
    input->outputs_allowed = APP_TRUE;
}

static int s34_mode_switch_resets_integral(FILE *out)
{
    ctrl_fuzzy_steering_state_t state;
    ctrl_fuzzy_steering_input_t input;
    int pass;

    ctrl_fuzzy_steering_init(&state);
    fill_steering_input(&input, TRACK_MODE_STRAIGHT, 160, 0, 520u, APP_STATE_GROUND_TRACK);
    ctrl_fuzzy_steering_update(&state, &input);
    ctrl_fuzzy_steering_update(&state, &input);
    input.mode = TRACK_MODE_SHARP_CURVE;
    ctrl_fuzzy_steering_update(&state, &input);
    pass = (state.integral == 0 && state.last_mode == TRACK_MODE_SHARP_CURVE);
    write_result(out, "S34", pass, pass ? "mode switch reset integral" : "mode switch left integral", state.integral, state.last_mode, state.gain.ki);
    return pass;
}

static int s35_line_lost_centers(FILE *out)
{
    ctrl_fuzzy_steering_state_t state;
    ctrl_fuzzy_steering_input_t input;
    s16 offset_before;
    s16 offset_after;
    int pass;

    ctrl_fuzzy_steering_init(&state);
    fill_steering_input(&input, TRACK_MODE_SHARP_CURVE, 700, 400, 520u, APP_STATE_GROUND_TRACK);
    offset_before = ctrl_fuzzy_steering_update(&state, &input);
    input.mode = TRACK_MODE_LINE_LOST;
    input.signal_quality = 0u;
    input.line_error_filtered = 0;
    input.error_rate = 0;
    offset_after = ctrl_fuzzy_steering_update(&state, &input);
    pass = (offset_before != 0 && offset_after == 0 && state.integral == 0 && state.previous_output_us == 0);
    write_result(out, "S35", pass, pass ? "line lost centers steering state" : "line lost retained dangerous steering", offset_before, offset_after, state.integral);
    return pass;
}

static int s36_crossing_without_route_is_conservative(FILE *out)
{
    track_mode_input_t route_input;
    ctrl_fuzzy_steering_state_t state;
    ctrl_fuzzy_steering_input_t steering_input;
    track_mode_t raw_mode;
    s16 offset;
    int pass;

    route_input.line_error = 0;
    route_input.error_rate = 0;
    route_input.line_quality = 960u;
    route_input.surface_state = SURFACE_GROUND;
    route_input.pitch_cdeg = 0;
    route_input.pitch_rate_cdeg_s = 0;
    route_input.speed_mm_s = 200;
    route_input.cross_confirmed = APP_TRUE;
    route_input.hex_confirmed = APP_FALSE;
    route_input.seesaw_confirmed = APP_FALSE;
    route_input.dt_ms = TASK_CONTROL_PERIOD_MS;
    raw_mode = track_route_profile_detect_raw(&route_input);

    ctrl_fuzzy_steering_init(&state);
    fill_steering_input(&steering_input, raw_mode, 0, 0, 960u, APP_STATE_GROUND_TRACK);
    offset = ctrl_fuzzy_steering_update(&state, &steering_input);
    pass = (track_route_profile_configured_count() == 0u &&
            raw_mode == TRACK_MODE_CROSSING &&
            offset == 0);
    write_result(out, "S36", pass, pass ? "crossing conservative without route" : "crossing assumed a turn", raw_mode, offset, track_route_profile_configured_count());
    return pass;
}

static int s37_omega_speed_lower(FILE *out)
{
    int straight_speed;
    int omega_speed;
    int sharp_speed;
    int pass;

    straight_speed = track_strategy_target_speed_for_mode(TRACK_MODE_STRAIGHT);
    omega_speed = track_strategy_target_speed_for_mode(TRACK_MODE_OMEGA);
    sharp_speed = track_strategy_target_speed_for_mode(TRACK_MODE_SHARP_CURVE);
    pass = (omega_speed < straight_speed && sharp_speed < straight_speed);
    write_result(out, "S37", pass, pass ? "omega and sharp speeds below straight" : "curve speed not below straight", straight_speed, omega_speed, sharp_speed);
    return pass;
}

static int s38_wall_integral_not_retained(FILE *out)
{
    ctrl_fuzzy_steering_state_t state;
    ctrl_fuzzy_steering_input_t input;
    int pass;

    ctrl_fuzzy_steering_init(&state);
    fill_steering_input(&input, TRACK_MODE_WALL, 260, 80, 520u, APP_STATE_WALL_TRACK);
    ctrl_fuzzy_steering_update(&state, &input);
    state.integral = 200;
    input.mode = TRACK_MODE_RECOVERY;
    input.app_state = APP_STATE_GROUND_RECOVERY;
    input.line_error_filtered = 0;
    input.error_rate = 0;
    ctrl_fuzzy_steering_update(&state, &input);
    pass = (state.integral == 0 && state.last_mode == TRACK_MODE_RECOVERY);
    write_result(out, "S38", pass, pass ? "recovery clears wall integral" : "wall integral retained after down transition", state.integral, state.last_mode, state.gain.ki);
    return pass;
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed;
    int total;

    if (argc != 2) {
        fputs("usage: fuzzy_pid_runner <summary.csv>\n", stderr);
        return 2;
    }
    out = fopen(argv[1], "w");
    if (out == 0) {
        fputs("failed to open output csv\n", stderr);
        return 2;
    }
    fputs("scenario,pass,metric_a,metric_b,metric_c,notes\n", out);

    passed = 0;
    total = 0;
    passed += s29_membership_boundaries(out); total++;
    passed += s30_rule_limits(out); total++;
    passed += s31_straight_less_than_sharp(out); total++;
    passed += s32_kp_not_reverse(out); total++;
    passed += s33_kd_high_on_fast_change(out); total++;
    passed += s34_mode_switch_resets_integral(out); total++;
    passed += s35_line_lost_centers(out); total++;
    passed += s36_crossing_without_route_is_conservative(out); total++;
    passed += s37_omega_speed_lower(out); total++;
    passed += s38_wall_integral_not_retained(out); total++;

    fclose(out);
    if (passed != total) {
        return 1;
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>

#include "host_bsp.h"
#include "../../App/app_config.h"
#include "../../App/app_output_arbitration.h"
#include "../../App/app_scheduler.h"
#include "../../App/app_state_machine.h"
#include "../../BSP/bsp_drive.h"
#include "../../BSP/bsp_emag.h"
#include "../../BSP/bsp_encoder.h"
#include "../../BSP/bsp_imu.h"
#include "../../BSP/bsp_power.h"
#include "../../BSP/bsp_steering.h"
#include "../../BSP/bsp_suction.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"
#include "../../Control/ctrl_line.h"
#include "../../Control/ctrl_speed.h"
#include "../../Control/ctrl_vehicle.h"

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

static emag_sample_t make_line_sample(u16 n0, u16 n1, u16 n2, u16 n3, u16 n4)
{
    emag_sample_t sample;
    sample.raw[0] = n0;
    sample.raw[1] = n1;
    sample.raw[2] = n2;
    sample.raw[3] = n3;
    sample.raw[4] = n4;
    sample.filtered[0] = n0;
    sample.filtered[1] = n1;
    sample.filtered[2] = n2;
    sample.filtered[3] = n3;
    sample.filtered[4] = n4;
    sample.norm[0] = n0;
    sample.norm[1] = n1;
    sample.norm[2] = n2;
    sample.norm[3] = n3;
    sample.norm[4] = n4;
    sample.line_error = 0;
    sample.line_quality = 0u;
    sample.signal_quality = 0u;
    sample.channel_count = 5u;
    sample.line_lost = APP_FALSE;
    sample.valid = APP_TRUE;
    return sample;
}

static int s39_weighted_centroid(FILE *out)
{
    emag_sample_t left;
    emag_sample_t center;
    emag_sample_t right;
    int pass;

    left = ctrl_line_update(make_line_sample(900u, 300u, 60u, 0u, 0u));
    center = ctrl_line_update(make_line_sample(0u, 80u, 900u, 80u, 0u));
    right = ctrl_line_update(make_line_sample(0u, 0u, 60u, 300u, 900u));
    pass = (left.line_error < 0 && center.line_error == 0 && right.line_error > 0);
    write_result(out, "S39", pass, pass ? "five-channel centroid sign ok" : "centroid sign mismatch", left.line_error, center.line_error, right.line_error);
    return pass;
}

static int s40_line_lost_low_energy(FILE *out)
{
    emag_sample_t low;
    int pass;

    low = ctrl_line_update(make_line_sample(1u, 1u, 1u, 1u, 1u));
    pass = (low.line_lost != APP_FALSE && low.valid == APP_FALSE && low.line_quality == 0u);
    write_result(out, "S40", pass, pass ? "low total energy marks line lost" : "low energy accepted", low.line_lost, low.valid, low.line_quality);
    return pass;
}

static encoder_sample_t make_encoder(s16 left_speed, s16 right_speed, u8 valid)
{
    encoder_sample_t encoder;
    encoder.left_count = 0l;
    encoder.right_count = 0l;
    encoder.left_speed_mm_s = left_speed;
    encoder.right_speed_mm_s = right_speed;
    encoder.valid = valid;
    return encoder;
}

static int s41_left_error_only(FILE *out)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    int pass;

    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, 100, 100, make_encoder(0, 100, APP_TRUE));
    pass = (output.left_native != 0 && output.right_native == 0);
    write_result(out, "S41", pass, pass ? "left speed error only changes left pwm" : "right pwm changed on left-only error", output.left_native, output.right_native, output.average_native);
    return pass;
}

static int s42_right_error_only(FILE *out)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    int pass;

    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, 100, 100, make_encoder(100, 0, APP_TRUE));
    pass = (output.left_native == 0 && output.right_native != 0);
    write_result(out, "S42", pass, pass ? "right speed error only changes right pwm" : "left pwm changed on right-only error", output.left_native, output.right_native, output.average_native);
    return pass;
}

static int s43_pi_limits(FILE *out)
{
    speed_pi_state_t state;
    s16 first;
    s16 last;
    u8 i;
    int pass;

    ctrl_speed_pi_init(&state);
    first = ctrl_speed_pi_update(&state, 500, 0, APP_TRUE);
    last = first;
    for (i = 0u; i < 40u; i++) {
        last = ctrl_speed_pi_update(&state, 500, 0, APP_TRUE);
    }
    pass = (first == SPEED_ACCEL_LIMIT &&
            last <= SPEED_OUTPUT_LIMIT &&
            state.integral <= SPEED_INTEGRAL_LIMIT &&
            state.previous_output <= SPEED_OUTPUT_LIMIT);
    write_result(out, "S43", pass, pass ? "pi integral output and accel limits ok" : "pi limiter failed", first, last, (int)state.integral);
    return pass;
}

static int s44_encoder_invalid_zero(FILE *out)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    int pass;

    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, 200, 200, make_encoder(0, 0, APP_TRUE));
    output = ctrl_speed_update_pair(&left_state, &right_state, 200, 200, make_encoder(0, 0, APP_FALSE));
    pass = (output.left_native == 0 &&
            output.right_native == 0 &&
            left_state.integral == 0l &&
            right_state.integral == 0l);
    write_result(out, "S44", pass, pass ? "encoder invalid zeros both drives" : "encoder invalid left drive active", output.left_native, output.right_native, output.average_native);
    return pass;
}

static int s45_dual_servo_sign(FILE *out)
{
    app_context_t ctx;
    int expected_left;
    int expected_right;
    int pass;

    app_state_machine_init(&ctx);
    ctx.speed_limit_mm_s = 100;
    ctx.left_drive_command_native = 0;
    ctx.right_drive_command_native = 0;
    ctx.steering_offset_us = 100;
    ctrl_vehicle_update(&ctx);
    expected_left = (int)STEERING_LEFT_CENTER_US + (STEERING_LEFT_SIGN * 100);
    expected_right = (int)STEERING_RIGHT_CENTER_US + (STEERING_RIGHT_SIGN * 100);
    pass = ((int)ctx.steering_left_pulse_us == expected_left &&
            (int)ctx.steering_right_pulse_us == expected_right);
    write_result(out, "S45", pass, pass ? "dual servo signs applied" : "dual servo sign output mismatch", ctx.steering_left_pulse_us, ctx.steering_right_pulse_us, ctx.steering_pulse_us);
    return pass;
}

static int s46_arbitration_preserves_independent_outputs(FILE *out)
{
    app_context_t ctx;
    int pass;

    app_state_machine_init(&ctx);
    ctx.app_state = APP_STATE_GROUND_TRACK;
    ctx.manual_arm = APP_TRUE;
    ctx.left_drive_command_native = 120;
    ctx.right_drive_command_native = 60;
    ctx.drive_command_native = 90;
    ctx.steering_left_pulse_us = (u16)(STEERING_LEFT_CENTER_US + 80u);
    ctx.steering_right_pulse_us = (u16)(STEERING_RIGHT_CENTER_US - 40u);
    ctx.steering_pulse_us = (u16)(((u32)ctx.steering_left_pulse_us + (u32)ctx.steering_right_pulse_us) / 2ul);
    app_output_arbitrate(&ctx);
    pass = (ctx.left_drive_command_native == 120 &&
            ctx.right_drive_command_native == 60 &&
            ctx.steering_left_pulse_us == (u16)(STEERING_LEFT_CENTER_US + 80u) &&
            ctx.steering_right_pulse_us == (u16)(STEERING_RIGHT_CENTER_US - 40u));
    write_result(out, "S46", pass, pass ? "arbitration preserves independent outputs" : "arbitration merged outputs", ctx.left_drive_command_native, ctx.right_drive_command_native, ctx.steering_left_pulse_us);
    return pass;
}

static int s47_safe_states_center(FILE *out)
{
    app_context_t ctx;
    int pass;

    app_state_machine_init(&ctx);
    ctx.app_state = APP_STATE_GROUND_TRACK;
    ctx.manual_arm = APP_FALSE;
    ctx.left_drive_command_native = 100;
    ctx.right_drive_command_native = 80;
    ctx.steering_left_pulse_us = STEERING_LEFT_MAX_US;
    ctx.steering_right_pulse_us = STEERING_RIGHT_MIN_US;
    app_output_arbitrate(&ctx);
    pass = (ctx.left_drive_command_native == 0 &&
            ctx.right_drive_command_native == 0 &&
            ctx.steering_left_pulse_us == STEERING_LEFT_CENTER_US &&
            ctx.steering_right_pulse_us == STEERING_RIGHT_CENTER_US);

    ctx.app_state = APP_STATE_FINISHED;
    ctx.manual_arm = APP_TRUE;
    ctx.left_drive_command_native = 100;
    ctx.right_drive_command_native = 80;
    app_output_arbitrate(&ctx);
    pass = pass && (ctx.left_drive_command_native == 0 &&
                    ctx.right_drive_command_native == 0 &&
                    ctx.steering_left_pulse_us == STEERING_LEFT_CENTER_US &&
                    ctx.steering_right_pulse_us == STEERING_RIGHT_CENTER_US);

    ctx.app_state = APP_STATE_HARD_FAULT;
    ctx.left_drive_command_native = 100;
    ctx.right_drive_command_native = 80;
    app_output_arbitrate(&ctx);
    pass = pass && (ctx.left_drive_command_native == 0 &&
                    ctx.right_drive_command_native == 0 &&
                    ctx.steering_left_pulse_us == STEERING_LEFT_CENTER_US &&
                    ctx.steering_right_pulse_us == STEERING_RIGHT_CENTER_US);
    write_result(out, "S47", pass, pass ? "safe states zero and center outputs" : "safe state left output active", ctx.left_drive_command_native, ctx.right_drive_command_native, ctx.steering_left_pulse_us);
    return pass;
}

static void init_host_system(app_context_t *ctx)
{
    host_bsp_reset();
    bsp_timebase_init();
    bsp_emag_init();
    bsp_imu_init();
    bsp_encoder_init();
    bsp_power_init();
    bsp_ui_init();
    bsp_drive_init();
    bsp_steering_init();
    bsp_suction_init();
    app_state_machine_init(ctx);
    app_scheduler_init();
}

static int s48_scheduler_uses_fuzzy_path(FILE *out)
{
    app_context_t ctx;
    host_sil_input_t input;
    u32 t;
    int pass;

    init_host_system(&ctx);
    for (t = 1ul; t <= 900ul; t++) {
        input.time_ms = t;
        input.manual_arm = (t >= 80ul) ? APP_TRUE : APP_FALSE;
        input.suction_authorize = APP_FALSE;
        input.transition_candidate = APP_FALSE;
        input.emag_valid = APP_TRUE;
        input.line_error = 0;
        input.signal_quality = 760u;
        input.emag_norm_valid = APP_TRUE;
        input.emag_norm[0] = 0u;
        input.emag_norm[1] = 0u;
        input.emag_norm[2] = 120u;
        input.emag_norm[3] = 280u;
        input.emag_norm[4] = 520u;
        input.imu_fresh = APP_TRUE;
        input.imu_id_ok = APP_TRUE;
        input.pitch_cdeg = 0;
        input.encoder_valid = APP_TRUE;
        input.left_count = (s32)t;
        input.right_count = (s32)t;
        input.left_speed_mm_s = 0;
        input.right_speed_mm_s = 0;
        input.power_ok = APP_TRUE;
        input.kill_switch = APP_FALSE;
        input.control_period_ok = APP_TRUE;
        input.force_app_state = -1;
        host_bsp_set_input(&input);
        bsp_timebase_on_tick_1ms();
        app_scheduler_run_due(&ctx, bsp_timebase_now_ms());
    }
    pass = (ctx.app_state == APP_STATE_GROUND_TRACK &&
            ctx.steering_offset_us != 0 &&
            ctx.fuzzy_kp > 0 &&
            ctx.steering_left_pulse_us != ctx.steering_right_pulse_us);
    write_result(out, "S48", pass, pass ? "scheduler fuzzy steering path affects output" : "scheduler steering path inactive", ctx.steering_offset_us, ctx.fuzzy_kp, ctx.track_mode);
    return pass;
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed;
    int total;

    if (argc != 2) {
        fputs("usage: real_control_chain_runner <summary.csv>\n", stderr);
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
    passed += s39_weighted_centroid(out); total++;
    passed += s40_line_lost_low_energy(out); total++;
    passed += s41_left_error_only(out); total++;
    passed += s42_right_error_only(out); total++;
    passed += s43_pi_limits(out); total++;
    passed += s44_encoder_invalid_zero(out); total++;
    passed += s45_dual_servo_sign(out); total++;
    passed += s46_arbitration_preserves_independent_outputs(out); total++;
    passed += s47_safe_states_center(out); total++;
    passed += s48_scheduler_uses_fuzzy_path(out); total++;

    fclose(out);
    if (passed != total) {
        return 1;
    }
    return 0;
}

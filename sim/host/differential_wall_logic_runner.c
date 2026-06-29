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
#include "../../BSP/bsp_fan_esc.h"
#include "../../BSP/bsp_imu.h"
#include "../../BSP/bsp_power.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"
#include "../../BSP/board_map.h"
#include "../../Control/ctrl_adhesion.h"
#include "../../Control/ctrl_differential_drive.h"
#include "../../Control/ctrl_fuzzy_turn.h"
#include "../../Control/ctrl_line.h"
#include "../../Control/ctrl_speed.h"
#include "../../Track/track_route_event.h"
#include "../../Track/track_surface_state.h"
#include "../../Track/track_wall_logic.h"

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

static emag_sample_t make_line_sample(u16 a, u16 b, u16 c, u16 d, u16 e)
{
    emag_sample_t sample;
    u8 i;
    sample.raw[0] = a;
    sample.raw[1] = b;
    sample.raw[2] = c;
    sample.raw[3] = d;
    sample.raw[4] = e;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        sample.filtered[i] = sample.raw[i];
        sample.norm[i] = sample.raw[i];
    }
    sample.line_error = 0;
    sample.line_quality = 0u;
    sample.signal_quality = 0u;
    sample.channel_count = EMAG_CHANNEL_COUNT;
    sample.line_lost = APP_FALSE;
    sample.valid = APP_TRUE;
    return sample;
}

static encoder_sample_t make_encoder(s16 left_speed, s16 right_speed, u8 valid)
{
    encoder_sample_t encoder;
    encoder.left_count = 0l;
    encoder.right_count = 0l;
    encoder.left_delta_counts = left_speed;
    encoder.right_delta_counts = right_speed;
    encoder.left_speed_counts_per_s = left_speed;
    encoder.right_speed_counts_per_s = right_speed;
    encoder.left_speed_mm_s = left_speed;
    encoder.right_speed_mm_s = right_speed;
    encoder.left_distance_mm = 0l;
    encoder.right_distance_mm = 0l;
    encoder.speed_mm_s_valid = APP_TRUE;
    encoder.progress_mm_valid = APP_TRUE;
    encoder.valid = valid;
    return encoder;
}

static attitude_sample_t make_attitude(s16 pitch_cdeg, u8 fresh)
{
    attitude_sample_t attitude;
    attitude.roll_cdeg = 0;
    attitude.pitch_cdeg = pitch_cdeg;
    attitude.pitch_rate_cdeg_s = 0;
    attitude.yaw_rate_cdeg_s = 0;
    attitude.accel_raw[0] = 0;
    attitude.accel_raw[1] = 0;
    attitude.accel_raw[2] = 16384;
    attitude.gyro_raw[0] = 0;
    attitude.gyro_raw[1] = 0;
    attitude.gyro_raw[2] = 0;
    attitude.timestamp_ms = 0ul;
    attitude.spi_ok = APP_TRUE;
    attitude.who_am_i = 0x6Bu;
    attitude.id_ok = APP_TRUE;
    attitude.fresh = fresh;
    return attitude;
}

static track_wall_input_t make_wall_input(u8 approach_event, s16 pitch_cdeg, u16 dt_ms)
{
    track_wall_input_t input;
    input.route_event.wall_approach_event = approach_event;
    input.route_event.crossing_event = APP_FALSE;
    input.route_event.omega_event = APP_FALSE;
    input.route_event.hex_loop_event = APP_FALSE;
    input.route_event.finish_event = APP_FALSE;
    input.attitude = make_attitude(pitch_cdeg, APP_TRUE);
    input.line_lost = APP_FALSE;
    input.adhesion_risk = 0u;
    input.dt_ms = dt_ms;
    return input;
}

static int d01_a_left_negative(FILE *out)
{
    emag_sample_t sample = ctrl_line_update(make_line_sample(900u, 0u, 0u, 0u, 0u));
    int pass = (sample.line_error == -2000 && sample.line_lost == APP_FALSE);
    write_result(out, "D01", pass, pass ? "A channel maps to negative error" : "A channel error not -2000", sample.line_error, sample.line_quality, sample.line_lost);
    return pass;
}

static int d02_c_center_zero(FILE *out)
{
    emag_sample_t sample = ctrl_line_update(make_line_sample(0u, 0u, 900u, 0u, 0u));
    int pass = (sample.line_error == 0 && sample.line_lost == APP_FALSE);
    write_result(out, "D02", pass, pass ? "C channel maps to zero error" : "C channel error not zero", sample.line_error, sample.line_quality, sample.line_lost);
    return pass;
}

static int d03_e_right_positive(FILE *out)
{
    emag_sample_t sample = ctrl_line_update(make_line_sample(0u, 0u, 0u, 0u, 900u));
    int pass = (sample.line_error == 2000 && sample.line_lost == APP_FALSE);
    write_result(out, "D03", pass, pass ? "E channel maps to positive error" : "E channel error not +2000", sample.line_error, sample.line_quality, sample.line_lost);
    return pass;
}

static int d04_line_lost_zero_mix(FILE *out)
{
    differential_drive_output_t output = ctrl_differential_drive_mix(180, 90, APP_FALSE);
    int pass = (output.left_target_mm_s == 0 && output.right_target_mm_s == 0 && output.valid == APP_FALSE);
    write_result(out, "D04", pass, pass ? "line lost disables differential target" : "line lost kept target speed", output.left_target_mm_s, output.right_target_mm_s, output.valid);
    return pass;
}

static int d05_positive_turn_direction(FILE *out)
{
    differential_drive_output_t output = ctrl_differential_drive_mix(160, 40, APP_TRUE);
    int expected_left = 160 + (DIFF_TURN_SIGN * 40);
    int expected_right = 160 - (DIFF_TURN_SIGN * 40);
    int pass = (output.left_target_mm_s == expected_left &&
                output.right_target_mm_s == expected_right &&
                output.left_target_mm_s != output.right_target_mm_s);
    write_result(out, "D05", pass, pass ? "positive turn delta changes wheel targets by sign" : "positive turn direction mismatch", output.left_target_mm_s, output.right_target_mm_s, DIFF_TURN_SIGN);
    return pass;
}

static int d06_negative_turn_direction(FILE *out)
{
    differential_drive_output_t output = ctrl_differential_drive_mix(160, -40, APP_TRUE);
    int expected_left = 160 - (DIFF_TURN_SIGN * 40);
    int expected_right = 160 + (DIFF_TURN_SIGN * 40);
    int pass = (output.left_target_mm_s == expected_left &&
                output.right_target_mm_s == expected_right &&
                output.left_target_mm_s != output.right_target_mm_s);
    write_result(out, "D06", pass, pass ? "negative turn delta reverses wheel target direction" : "negative turn direction mismatch", output.left_target_mm_s, output.right_target_mm_s, DIFF_TURN_SIGN);
    return pass;
}

static int d07_speed_pi_independent(FILE *out)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, 120, 120, make_encoder(0, 120, APP_TRUE));
    {
        int pass = (output.left_native != 0 && output.right_native == 0);
        write_result(out, "D07", pass, pass ? "left wheel speed error only changes left native output" : "speed PI channels coupled", output.left_native, output.right_native, output.average_native);
        return pass;
    }
}

static int d08_encoder_invalid_zero(FILE *out)
{
    speed_pi_state_t left_state;
    speed_pi_state_t right_state;
    speed_pi_output_t output;
    ctrl_speed_pi_init(&left_state);
    ctrl_speed_pi_init(&right_state);
    output = ctrl_speed_update_pair(&left_state, &right_state, 120, 120, make_encoder(0, 0, APP_FALSE));
    {
        int pass = (output.left_native == 0 && output.right_native == 0);
        write_result(out, "D08", pass, pass ? "encoder invalid zeros native outputs" : "encoder invalid kept output", output.left_native, output.right_native, output.average_native);
        return pass;
    }
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
    bsp_fan_esc_init();
    app_state_machine_init(ctx);
    app_scheduler_init();
}

static int d09_scheduler_no_steering_call(FILE *out)
{
    app_context_t ctx;
    host_sil_input_t input;
    u32 t;
    int pass;

    init_host_system(&ctx);
    for (t = 1ul; t <= 700ul; t++) {
        input.time_ms = t;
        input.manual_arm = (t >= 80ul) ? APP_TRUE : APP_FALSE;
        input.suction_authorize = APP_FALSE;
        input.transition_candidate = APP_FALSE;
        input.route_event.wall_approach_event = APP_FALSE;
        input.route_event.crossing_event = APP_FALSE;
        input.route_event.omega_event = APP_FALSE;
        input.route_event.hex_loop_event = APP_FALSE;
        input.route_event.finish_event = APP_FALSE;
        input.emag_valid = APP_TRUE;
        input.line_error = 0;
        input.signal_quality = 900u;
        input.emag_norm_valid = APP_TRUE;
        input.emag_norm[0] = 0u;
        input.emag_norm[1] = 0u;
        input.emag_norm[2] = 900u;
        input.emag_norm[3] = 0u;
        input.emag_norm[4] = 0u;
        input.imu_fresh = APP_TRUE;
        input.imu_id_ok = APP_TRUE;
        input.pitch_cdeg = 0;
        input.pitch_rate_cdeg_s = 0;
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

    pass = (host_bsp_steering_apply_count() == 0u &&
            ctx.left_speed_target_mm_s == ctx.right_speed_target_mm_s &&
            bsp_drive_last_left_native() == bsp_drive_last_right_native());
    write_result(out, "D09", pass, pass ? "scheduler main path does not call steering BSP" : "scheduler still called steering BSP", (int)host_bsp_steering_apply_count(), bsp_drive_last_left_native(), bsp_drive_last_right_native());
    return pass;
}

static int w01_approach_to_precharge(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, 0, TASK_CONTROL_PERIOD_MS);
    output = track_wall_logic_update(&logic, &input);
    output = track_wall_logic_update(&logic, &input);
    {
        int pass = (output.state == TRACK_WALL_FAN_PRECHARGE && output.fan_state == FAN_ESC_PRECHARGE);
        write_result(out, "W01", pass, pass ? "wall approach enters fan precharge" : "wall approach did not enter precharge", output.state, output.fan_state, output.speed_limit_mm_s);
        return pass;
    }
}

static int w02_precharge_blocks_transition(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, IMU_WALL_ENTER_CDEG, TASK_CONTROL_PERIOD_MS);
    output = track_wall_logic_update(&logic, &input);
    output = track_wall_logic_update(&logic, &input);
    {
        int pass = (output.state == TRACK_WALL_FAN_PRECHARGE);
        write_result(out, "W02", pass, pass ? "transition is blocked until precharge time ends" : "transition happened before precharge dwell", output.state, output.state_elapsed_ms, output.fan_state);
        return pass;
    }
}

static int w03_precharge_then_transition_up(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    u16 elapsed;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, IMU_WALL_ENTER_CDEG, TASK_CONTROL_PERIOD_MS);
    output = track_wall_logic_update(&logic, &input);
    for (elapsed = 0u; elapsed < (u16)(FAN_PRECHARGE_TIME_MS + 20u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        output = track_wall_logic_update(&logic, &input);
    }
    {
        int pass = (output.state == TRACK_WALL_TRANSITION_UP && output.fan_state == FAN_ESC_HOLD);
        write_result(out, "W03", pass, pass ? "precharge dwell enters transition-up before wall confirm" : "transition-up not reached after precharge", output.state, output.state_elapsed_ms, output.fan_state);
        return pass;
    }
}

static int w04_wall_track_holds_fan(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    u16 elapsed;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, IMU_WALL_ENTER_CDEG, TASK_CONTROL_PERIOD_MS);
    for (elapsed = 0u; elapsed < (u16)(FAN_PRECHARGE_TIME_MS + IMU_TRANSITION_CONFIRM_MS + IMU_TRANSITION_CONFIRM_MS + 80u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        output = track_wall_logic_update(&logic, &input);
    }
    {
        int pass = (output.state == TRACK_WALL_WALL_TRACK && output.fan_state == FAN_ESC_HOLD);
        write_result(out, "W04", pass, pass ? "wall confirmed and fan hold maintained" : "wall track hold not reached", output.state, output.state_elapsed_ms, output.fan_state);
        return pass;
    }
}

static int w05_risk_boost_returns_hold(FILE *out)
{
    ctrl_adhesion_state_t adhesion;
    fan_esc_command_t cmd;
    ctrl_adhesion_init(&adhesion);
    ctrl_adhesion_update(&adhesion, TRACK_WALL_WALL_TRACK, 900u, TASK_CONTROL_PERIOD_MS, &cmd);
    {
        u16 elapsed;
        int boost_seen = (cmd.state == FAN_ESC_BOOST);
        for (elapsed = 0u; elapsed < (u16)(FAN_BOOST_TIME_MS + 40u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
            ctrl_adhesion_update(&adhesion, TRACK_WALL_WALL_TRACK, 0u, TASK_CONTROL_PERIOD_MS, &cmd);
        }
        {
            int pass = (boost_seen && cmd.state == FAN_ESC_HOLD);
            write_result(out, "W05", pass, pass ? "risk boost is short and returns hold" : "boost did not return hold", boost_seen, cmd.state, cmd.request_us);
            return pass;
        }
    }
}

static int w06_down_keeps_hold(FILE *out)
{
    ctrl_adhesion_state_t adhesion;
    fan_esc_command_t cmd;
    ctrl_adhesion_init(&adhesion);
    ctrl_adhesion_update(&adhesion, TRACK_WALL_TRANSITION_DOWN, 0u, TASK_CONTROL_PERIOD_MS, &cmd);
    {
        int pass = (cmd.state == FAN_ESC_HOLD && cmd.request_us == FAN_HOLD_US);
        write_result(out, "W06", pass, pass ? "transition down keeps fan hold" : "transition down dropped fan early", cmd.state, cmd.request_us, cmd.output_us);
        return pass;
    }
}

static int w07_ground_recovery_ramp_down(FILE *out)
{
    ctrl_adhesion_state_t adhesion;
    fan_esc_command_t cmd;
    u16 elapsed;
    ctrl_adhesion_init(&adhesion);
    ctrl_adhesion_update(&adhesion, TRACK_WALL_GROUND_RECOVERY, 0u, TASK_CONTROL_PERIOD_MS, &cmd);
    for (elapsed = 0u; elapsed < (u16)(IMU_GROUND_CONFIRM_MS + FAN_RAMP_DOWN_PERIOD_MS + 20u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        ctrl_adhesion_update(&adhesion, TRACK_WALL_GROUND_RECOVERY, 0u, TASK_CONTROL_PERIOD_MS, &cmd);
    }
    {
        int pass = (cmd.state == FAN_ESC_RAMP_DOWN || cmd.state == FAN_ESC_OFF);
        write_result(out, "W07", pass, pass ? "ground recovery confirms before ramp down" : "ground recovery failed to ramp down", cmd.state, cmd.request_us, cmd.output_us);
        return pass;
    }
}

static int w08_stale_wall_failsafe(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    u16 elapsed;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, IMU_WALL_ENTER_CDEG, TASK_CONTROL_PERIOD_MS);
    for (elapsed = 0u; elapsed < (u16)(FAN_PRECHARGE_TIME_MS + IMU_TRANSITION_CONFIRM_MS + IMU_TRANSITION_CONFIRM_MS + 80u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        output = track_wall_logic_update(&logic, &input);
    }
    input.attitude.fresh = APP_FALSE;
    output = track_wall_logic_update(&logic, &input);
    {
        int pass = (output.state == TRACK_WALL_FAILSAFE_HOLD && output.fan_state == FAN_ESC_FAILSAFE_HOLD && output.drive_allowed == APP_FALSE);
        write_result(out, "W08", pass, pass ? "IMU stale on wall enters failsafe hold" : "IMU stale did not enter wall failsafe", output.state, output.fan_state, output.drive_allowed);
        return pass;
    }
}

static int w09_down_no_rebound_to_up(FILE *out)
{
    track_wall_logic_t logic;
    track_wall_output_t output;
    track_wall_input_t input;
    u16 elapsed;
    track_wall_logic_init(&logic);
    input = make_wall_input(APP_TRUE, IMU_WALL_ENTER_CDEG, TASK_CONTROL_PERIOD_MS);
    for (elapsed = 0u; elapsed < (u16)(FAN_PRECHARGE_TIME_MS + IMU_TRANSITION_CONFIRM_MS + IMU_TRANSITION_CONFIRM_MS + 80u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        output = track_wall_logic_update(&logic, &input);
    }
    input.attitude.pitch_cdeg = IMU_WALL_EXIT_CDEG - 500;
    for (elapsed = 0u; elapsed < (u16)(IMU_TRANSITION_CONFIRM_MS + 20u); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        output = track_wall_logic_update(&logic, &input);
    }
    input.attitude.pitch_cdeg = IMU_WALL_ENTER_CDEG + 500;
    output = track_wall_logic_update(&logic, &input);
    {
        int pass = (output.state != TRACK_WALL_TRANSITION_UP);
        write_result(out, "W09", pass, pass ? "transition down does not rebound to up" : "transition down rebounded to up", output.state, output.state_elapsed_ms, output.fan_state);
        return pass;
    }
}

static int w10_physical_output_disabled(FILE *out)
{
    fan_esc_command_t cmd;
    fan_esc_command_t last;
    bsp_fan_esc_init();
    cmd.state = FAN_ESC_BOOST;
    cmd.request_us = FAN_BOOST_US;
    cmd.output_us = FAN_BOOST_US;
    cmd.mapped = BOARD_FAN_PWM_MAPPED;
    cmd.physical_enabled = FAN_ESC_PHYSICAL_OUTPUT_ENABLE;
    bsp_fan_esc_apply(&cmd);
    last = bsp_fan_esc_last_command();
    {
        int pass = (bsp_fan_esc_last_output_us() == 0u &&
                    last.request_us == FAN_BOOST_US &&
                    last.physical_enabled == APP_FALSE);
        write_result(out, "W10", pass, pass ? "physical fan output remains zero while logical request changes" : "fan physical output enabled unexpectedly", last.request_us, bsp_fan_esc_last_output_us(), last.physical_enabled);
        return pass;
    }
}

static int w11_safe_states_zero_drive_fan(FILE *out)
{
    app_context_t ctx;
    fan_esc_command_t fan;
    app_state_machine_init(&ctx);
    bsp_fan_esc_init();
    ctx.app_state = APP_STATE_FINISHED;
    ctx.manual_arm = APP_TRUE;
    ctx.left_drive_command_native = 180;
    ctx.right_drive_command_native = 160;
    ctx.fan_cmd.state = FAN_ESC_HOLD;
    ctx.fan_cmd.request_us = FAN_HOLD_US;
    app_output_arbitrate(&ctx);
    bsp_fan_esc_apply(&ctx.fan_cmd);
    fan = bsp_fan_esc_last_command();
    {
        int pass = (ctx.left_drive_command_native == 0 &&
                    ctx.right_drive_command_native == 0 &&
                    fan.state == FAN_ESC_OFF &&
                    bsp_fan_esc_last_output_us() == 0u);
        write_result(out, "W11", pass, pass ? "finished state zeros drive and real fan output" : "safe state kept drive or fan active", ctx.left_drive_command_native, ctx.right_drive_command_native, bsp_fan_esc_last_output_us());
        return pass;
    }
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed = 0;
    int total = 0;

    if (argc != 2) {
        fputs("usage: differential_wall_logic_runner <summary.csv>\n", stderr);
        return 2;
    }

    out = fopen(argv[1], "w");
    if (out == 0) {
        fputs("failed to open output csv\n", stderr);
        return 2;
    }
    fputs("scenario,pass,metric_a,metric_b,metric_c,notes\n", out);

    passed += d01_a_left_negative(out); total++;
    passed += d02_c_center_zero(out); total++;
    passed += d03_e_right_positive(out); total++;
    passed += d04_line_lost_zero_mix(out); total++;
    passed += d05_positive_turn_direction(out); total++;
    passed += d06_negative_turn_direction(out); total++;
    passed += d07_speed_pi_independent(out); total++;
    passed += d08_encoder_invalid_zero(out); total++;
    passed += d09_scheduler_no_steering_call(out); total++;
    passed += w01_approach_to_precharge(out); total++;
    passed += w02_precharge_blocks_transition(out); total++;
    passed += w03_precharge_then_transition_up(out); total++;
    passed += w04_wall_track_holds_fan(out); total++;
    passed += w05_risk_boost_returns_hold(out); total++;
    passed += w06_down_keeps_hold(out); total++;
    passed += w07_ground_recovery_ramp_down(out); total++;
    passed += w08_stale_wall_failsafe(out); total++;
    passed += w09_down_no_rebound_to_up(out); total++;
    passed += w10_physical_output_disabled(out); total++;
    passed += w11_safe_states_zero_drive_fan(out); total++;

    fclose(out);
    return (passed == total) ? 0 : 1;
}

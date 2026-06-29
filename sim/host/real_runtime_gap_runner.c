#include <stdio.h>
#include <stdlib.h>

#include "../../App/app_config.h"
#include "../../App/app_state_machine.h"
#include "../../BSP/bsp_emag.h"
#include "../../BSP/bsp_encoder.h"
#include "../../BSP/bsp_fan_esc.h"
#include "../../BSP/bsp_imu.h"
#include "../../BSP/bsp_power.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"
#include "../../Control/ctrl_adhesion.h"
#include "../../Control/ctrl_line.h"
#include "../../Track/track_route_event.h"
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

static attitude_sample_t attitude_with_pitch(s16 pitch_cdeg)
{
    attitude_sample_t sample;
    sample.roll_cdeg = 0;
    sample.pitch_cdeg = pitch_cdeg;
    sample.pitch_rate_cdeg_s = 0;
    sample.yaw_rate_cdeg_s = 0;
    sample.timestamp_ms = bsp_timebase_now_ms();
    sample.id_ok = APP_TRUE;
    sample.fresh = APP_TRUE;
    sample.spi_ok = APP_TRUE;
    sample.who_am_i = LSM6DSR_WHO_AM_I_EXPECTED;
    sample.accel_raw[0] = 0;
    sample.accel_raw[1] = 0;
    sample.accel_raw[2] = 16384;
    sample.gyro_raw[0] = 0;
    sample.gyro_raw[1] = 0;
    sample.gyro_raw[2] = 0;
    return sample;
}

static int runtime01_power_guard(FILE *out)
{
    power_sample_t sample;
    int pass;

    bsp_power_init();
    bsp_power_host_set_raw(0u, APP_TRUE);
    sample = bsp_power_read_sample();
    pass = (POWER_GUARD_ENABLE == 0 &&
            sample.power_good != APP_FALSE &&
            sample.calibration_valid == APP_FALSE &&
            sample.power_uncalibrated != APP_FALSE);
    write_result(out, "RUNTIME01", pass, pass ? "power guard disabled allows safe-ready path with POWER_UNCALIBRATED telemetry" : "power guard still blocks or fakes calibration", sample.power_good, sample.calibration_valid, sample.power_uncalibrated);
    return pass;
}

static int runtime02_arm_to_ground(FILE *out)
{
    app_context_t ctx;
    int pass;
    u8 i;

    bsp_ui_init();
    bsp_ui_host_set_arm(APP_TRUE);
    app_state_machine_init(&ctx);
    ctx.health.power_ok = APP_TRUE;
    ctx.health.emag_ok = APP_TRUE;
    ctx.health.imu_fresh = APP_TRUE;
    ctx.health.encoder_ok = APP_TRUE;
    ctx.health.control_period_ok = APP_TRUE;
    ctx.surface_state = SURFACE_GROUND;
    for (i = 0u; i < 8u; i++) {
        ctx.manual_arm = bsp_ui_manual_arm_requested();
        app_state_machine_step(&ctx, TASK_CONTROL_PERIOD_MS);
    }
    pass = (ctx.app_state == APP_STATE_GROUND_TRACK);
    write_result(out, "RUNTIME02", pass, pass ? "arm input reaches ground track" : "arm path did not reach ground track", ctx.app_state, ctx.manual_arm, ctx.state_elapsed_ms);
    return pass;
}

static int runtime03_five_adc_valid(FILE *out)
{
    emag_sample_t sample;
    int pass;

    bsp_emag_init();
    bsp_emag_host_set_raw(600u, 700u, 900u, 700u, 600u, APP_TRUE);
    sample = ctrl_line_update(bsp_emag_read());
    pass = (sample.valid != APP_FALSE &&
            sample.channel_count == EMAG_CHANNEL_COUNT &&
            sample.line_lost == APP_FALSE);
    write_result(out, "RUNTIME03", pass, pass ? "five-channel ADC valid sample remains control-valid" : "five-channel ADC sample rejected", sample.valid, sample.line_lost, sample.line_quality);
    return pass;
}

static int runtime04_imu_valid(FILE *out)
{
    attitude_sample_t sample;
    int pass;

    bsp_imu_init();
    bsp_imu_host_set_sample(APP_TRUE, LSM6DSR_WHO_AM_I_EXPECTED, 0, 0, 16384, 10, 20, 30);
    sample = bsp_imu_read();
    pass = (sample.fresh != APP_FALSE &&
            sample.id_ok != APP_FALSE &&
            sample.spi_ok != APP_FALSE &&
            sample.who_am_i == LSM6DSR_WHO_AM_I_EXPECTED);
    write_result(out, "RUNTIME04", pass, pass ? "IMU valid sample reports fresh/id_ok" : "IMU valid sample not accepted", sample.fresh, sample.id_ok, sample.who_am_i);
    return pass;
}

static int runtime05_imu_spi_failure(FILE *out)
{
    attitude_sample_t sample;
    int pass;

    bsp_imu_init();
    bsp_imu_host_set_sample(APP_FALSE, 0u, 0, 0, 0, 0, 0, 0);
    sample = bsp_imu_read();
    pass = (sample.fresh == APP_FALSE &&
            sample.id_ok == APP_FALSE &&
            sample.spi_ok == APP_FALSE);
    write_result(out, "RUNTIME05", pass, pass ? "IMU SPI failure explicit" : "IMU SPI failure was hidden", sample.fresh, sample.id_ok, sample.spi_ok);
    return pass;
}

static int runtime06_encoder_units(FILE *out)
{
    encoder_sample_t sample;
    int pass;

    bsp_encoder_init();
    bsp_encoder_host_set_delta_counts(25, 25, APP_TRUE);
    sample = bsp_encoder_read();
    pass = (sample.left_delta_counts == 25 &&
            sample.left_speed_counts_per_s > 0 &&
            sample.speed_mm_s_valid == APP_FALSE &&
            sample.left_speed_mm_s == 0);
    write_result(out, "RUNTIME06", pass, pass ? "count/s and mm/s stay separated while scale is uncalibrated" : "encoder mixed count/s into mm/s", sample.left_delta_counts, sample.left_speed_counts_per_s, sample.left_speed_mm_s);
    return pass;
}

static int runtime07_progress_uncalibrated(FILE *out)
{
    encoder_sample_t sample;
    track_route_event_t event;
    int pass;

    bsp_encoder_init();
    bsp_encoder_host_set_delta_counts(500, 500, APP_TRUE);
    sample = bsp_encoder_read();
    event = track_route_event_from_progress(&sample);
    pass = (event.wall_approach_event == APP_FALSE &&
            track_route_event_progress_status() != ROUTE_PROGRESS_READY);
    write_result(out, "RUNTIME07", pass, pass ? "uncalibrated progress script cannot trigger wall event" : "uncalibrated progress triggered event", event.wall_approach_event, track_route_event_progress_status(), sample.progress_mm_valid);
    return pass;
}

static track_wall_output_t update_wall(track_wall_logic_t *logic,
                                       track_route_event_t route_event,
                                       s16 pitch,
                                       u16 dt_ms)
{
    track_wall_input_t input;
    input.route_event = route_event;
    input.attitude = attitude_with_pitch(pitch);
    input.line_lost = APP_FALSE;
    input.adhesion_risk = 0u;
    input.dt_ms = dt_ms;
    return track_wall_logic_update(logic, &input);
}

static int runtime08_wall_latch(FILE *out)
{
    track_wall_logic_t logic;
    track_route_event_t event;
    track_wall_output_t wall_out;
    int pass;

    track_wall_logic_init(&logic);
    event = track_route_event_none();
    event.wall_approach_event = APP_TRUE;
    wall_out = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    event.wall_approach_event = APP_FALSE;
    wall_out = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    pass = (logic.wall_approach_latched != APP_FALSE &&
            wall_out.state == TRACK_WALL_FAN_PRECHARGE);
    write_result(out, "RUNTIME08", pass, pass ? "wall approach event is latched" : "wall approach fell back after event drop", wall_out.state, logic.wall_approach_latched, logic.wall_cycle_active);
    return pass;
}

static int runtime09_precharge_speed(FILE *out)
{
    track_wall_logic_t logic;
    track_route_event_t event;
    track_wall_output_t result;
    int pass;

    track_wall_logic_init(&logic);
    event = track_route_event_none();
    event.wall_approach_event = APP_TRUE;
    result = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    result = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    pass = (result.state == TRACK_WALL_FAN_PRECHARGE &&
            result.speed_limit_mm_s > 0);
    write_result(out, "RUNTIME09", pass, pass ? "fan precharge keeps nonzero entry speed" : "fan precharge speed is zero", result.state, result.speed_limit_mm_s, result.fan_state);
    return pass;
}

static int runtime10_precharge_timeout_transition_up(FILE *out)
{
    track_wall_logic_t logic;
    track_route_event_t event;
    track_wall_output_t result;
    u16 elapsed;
    int pass;

    track_wall_logic_init(&logic);
    event = track_route_event_none();
    event.wall_approach_event = APP_TRUE;
    result = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    for (elapsed = 0u; elapsed <= (u16)(FAN_PRECHARGE_TIME_MS + TASK_CONTROL_PERIOD_MS); elapsed = (u16)(elapsed + TASK_CONTROL_PERIOD_MS)) {
        event.wall_approach_event = APP_FALSE;
        result = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    }
    pass = (result.state == TRACK_WALL_TRANSITION_UP);
    write_result(out, "RUNTIME10", pass, pass ? "precharge timeout enters transition-up unconditionally" : "precharge did not enter transition-up", result.state, result.state_elapsed_ms, logic.wall_approach_latched);
    return pass;
}

static int runtime11_failsafe_hold_fan(FILE *out)
{
    ctrl_adhesion_state_t state;
    fan_esc_command_t command;
    int pass;

    ctrl_adhesion_init(&state);
    ctrl_adhesion_set_physical_active(&state, APP_TRUE);
    ctrl_adhesion_update(&state,
                         TRACK_WALL_FAILSAFE_HOLD,
                         1000u,
                         TASK_ADHESION_PERIOD_MS,
                         &command);
    pass = (command.state == FAN_ESC_FAILSAFE_HOLD &&
            command.request_us == FAN_HOLD_US);
    write_result(out, "RUNTIME11", pass, pass ? "wall failsafe requests fan hold logically" : "wall failsafe cut fan request", command.state, command.request_us, command.output_us);
    return pass;
}

static int runtime12_esc_arm_waits_physical(FILE *out)
{
    ctrl_adhesion_state_t state;
    fan_esc_command_t command;
    u16 elapsed;
    int pass;

    ctrl_adhesion_init(&state);
    ctrl_adhesion_set_physical_active(&state, APP_FALSE);
    for (elapsed = 0u; elapsed <= (u16)(FAN_ESC_ARM_TIME_MS + TASK_ADHESION_PERIOD_MS); elapsed = (u16)(elapsed + TASK_ADHESION_PERIOD_MS)) {
        ctrl_adhesion_update(&state,
                             TRACK_WALL_GROUND_TRACK,
                             0u,
                             TASK_ADHESION_PERIOD_MS,
                             &command);
    }
    pass = (state.real_esc_armed == APP_FALSE);
    write_result(out, "RUNTIME12", pass, pass ? "ESC arm timer waits for physical PWM active" : "ESC arm timer faked real arm", state.state, state.real_esc_armed, state.state_elapsed_ms);
    return pass;
}

static int runtime13_new_cycle_resets_flags(FILE *out)
{
    track_wall_logic_t logic;
    track_route_event_t event;
    int pass;

    track_wall_logic_init(&logic);
    logic.ground_recovery_seen = APP_TRUE;
    logic.finish_event_consumed = APP_TRUE;
    event = track_route_event_none();
    event.wall_approach_event = APP_TRUE;
    update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    pass = (logic.ground_recovery_seen == APP_FALSE &&
            logic.finish_event_consumed == APP_FALSE &&
            logic.wall_cycle_active != APP_FALSE);
    write_result(out, "RUNTIME13", pass, pass ? "new wall cycle resets recovery flags" : "new wall cycle kept stale flags", logic.ground_recovery_seen, logic.finish_event_consumed, logic.wall_cycle_active);
    return pass;
}

static int runtime14_finish_consumed_once(FILE *out)
{
    track_wall_logic_t logic;
    track_route_event_t event;
    track_wall_output_t first;
    track_wall_output_t second;
    int pass;

    track_wall_logic_init(&logic);
    logic.state = TRACK_WALL_GROUND_RECOVERY;
    logic.wall_cycle_active = APP_TRUE;
    logic.ground_recovery_seen = APP_TRUE;
    event = track_route_event_none();
    event.finish_event = APP_TRUE;
    first = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    second = update_wall(&logic, event, 0, TASK_CONTROL_PERIOD_MS);
    pass = (first.finish_ready != APP_FALSE &&
            second.finish_ready == APP_FALSE &&
            logic.finish_event_consumed != APP_FALSE);
    write_result(out, "RUNTIME14", pass, pass ? "finish event consumed once" : "finish event repeated or missed", first.finish_ready, second.finish_ready, logic.finish_event_consumed);
    return pass;
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed;
    int total;

    if (argc != 2) {
        fputs("usage: real_runtime_gap_runner <summary.csv>\n", stderr);
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
    passed += runtime01_power_guard(out); total++;
    passed += runtime02_arm_to_ground(out); total++;
    passed += runtime03_five_adc_valid(out); total++;
    passed += runtime04_imu_valid(out); total++;
    passed += runtime05_imu_spi_failure(out); total++;
    passed += runtime06_encoder_units(out); total++;
    passed += runtime07_progress_uncalibrated(out); total++;
    passed += runtime08_wall_latch(out); total++;
    passed += runtime09_precharge_speed(out); total++;
    passed += runtime10_precharge_timeout_transition_up(out); total++;
    passed += runtime11_failsafe_hold_fan(out); total++;
    passed += runtime12_esc_arm_waits_physical(out); total++;
    passed += runtime13_new_cycle_resets_flags(out); total++;
    passed += runtime14_finish_consumed_once(out); total++;

    fclose(out);
    return (passed == total) ? 0 : 1;
}

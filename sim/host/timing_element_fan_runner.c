#include <stdio.h>
#include <string.h>

#include "../../App/app_config.h"
#include "../../App/app_control_tick.h"
#include "../../App/app_scheduler.h"
#include "../../App/app_state_machine.h"
#include "../../BSP/bsp_drive.h"
#include "../../BSP/bsp_emag.h"
#include "../../BSP/bsp_fan_esc.h"
#include "../../BSP/bsp_timing_scope.h"
#include "../../BSP/bsp_timebase.h"
#include "../../Control/ctrl_adhesion.h"
#include "../../Track/track_features.h"
#include "host_bsp.h"

static void write_result(FILE *out,
                         const char *scenario,
                         int pass,
                         const char *detail,
                         long v1,
                         long v2,
                         long v3)
{
    fprintf(out, "%s,%s,%s,%ld,%ld,%ld\n",
            scenario,
            pass ? "PASS" : "FAIL",
            detail,
            v1,
            v2,
            v3);
}

static void base_input(host_sil_input_t *input)
{
    memset(input, 0, sizeof(*input));
    input->manual_arm = APP_TRUE;
    input->suction_authorize = APP_FALSE;
    input->route_event = track_route_event_none();
    input->emag_valid = APP_TRUE;
    input->signal_quality = LINE_QUALITY_MIN;
    input->emag_norm_valid = APP_TRUE;
    input->emag_norm[0] = 0u;
    input->emag_norm[1] = 0u;
    input->emag_norm[2] = LINE_QUALITY_MIN;
    input->emag_norm[3] = 0u;
    input->emag_norm[4] = 0u;
    input->imu_fresh = APP_TRUE;
    input->imu_id_ok = APP_TRUE;
    input->encoder_valid = APP_TRUE;
    input->power_ok = APP_TRUE;
    input->control_period_ok = APP_TRUE;
    input->force_app_state = APP_STATE_GROUND_TRACK;
}

static void setup_context(app_context_t *ctx)
{
    host_sil_input_t input;

    host_bsp_reset();
    bsp_timebase_init();
    bsp_drive_init();
    bsp_fan_esc_init();
    base_input(&input);
    host_bsp_set_input(&input);
    app_state_machine_init(ctx);
    ctx->manual_arm = APP_TRUE;
    ctx->surface_state = SURFACE_GROUND;
    app_scheduler_init();
    app_control_tick_bind_context(ctx);
#ifdef HOST_SIL_REAL_EMAG_SCAN
    bsp_emag_init();
    bsp_emag_host_set_raw(0u, 0u, LINE_QUALITY_MIN, 0u, 0u, APP_TRUE);
#endif
}

static emag_sample_t make_emag(u16 a, u16 b, u16 c, u16 d, u16 e, u16 quality, u8 valid)
{
    emag_sample_t sample;

    memset(&sample, 0, sizeof(sample));
    sample.raw[0] = a;
    sample.raw[1] = b;
    sample.raw[2] = c;
    sample.raw[3] = d;
    sample.raw[4] = e;
    sample.filtered[0] = a;
    sample.filtered[1] = b;
    sample.filtered[2] = c;
    sample.filtered[3] = d;
    sample.filtered[4] = e;
    sample.norm[0] = a;
    sample.norm[1] = b;
    sample.norm[2] = c;
    sample.norm[3] = d;
    sample.norm[4] = e;
    sample.line_quality = quality;
    sample.signal_quality = quality;
    sample.channel_count = EMAG_CHANNEL_COUNT;
    sample.valid = valid;
    sample.line_lost = (valid == APP_FALSE) ? APP_TRUE : APP_FALSE;
    return sample;
}

static int timer_fix01_frequency_relationship(FILE *out)
{
    int pass;

    pass = (SENSOR_ADC_TICK_HZ == 1000u &&
            SENSOR_FRAME_HZ == (SENSOR_ADC_TICK_HZ / EMAG_CHANNEL_COUNT) &&
            SENSOR_FRAME_HZ == 200u &&
            CONTROL_PID_HZ == SENSOR_FRAME_HZ);
    write_result(out, "TIMER_FIX01", pass, "1000 Hz ADC tick derives 200 Hz frame and PID rates", SENSOR_ADC_TICK_HZ, SENSOR_FRAME_HZ, CONTROL_PID_HZ);
    return pass;
}

static int timer_fix02_pid_not_above_frame(FILE *out)
{
    int pass;

    pass = (CONTROL_PID_HZ <= SENSOR_FRAME_HZ &&
            TASK_FAST_SENSOR_PERIOD_MS == 1u &&
            TASK_CONTROL_PERIOD_MS == 5u);
    write_result(out, "TIMER_FIX02", pass, "control PID rate does not exceed complete frame rate", TASK_FAST_SENSOR_PERIOD_MS, TASK_CONTROL_PERIOD_MS, CONTROL_PID_HZ);
    return pass;
}

static int timer_fix03_five_ticks_one_sequence(FILE *out)
{
    app_context_t ctx;
    app_control_tick_stats_t stats;
    u8 i;
    int pass;

    setup_context(&ctx);
    for (i = 0u; i < (EMAG_CHANNEL_COUNT - 1u); i++) {
        app_control_tick_sensor_isr(&ctx, i);
    }
    stats = app_control_tick_stats();
    pass = (stats.sensor_frame_sequence == 0u && stats.speed_pi_pair_calls == 0u);
    app_control_tick_sensor_isr(&ctx, EMAG_CHANNEL_COUNT);
    stats = app_control_tick_stats();
    pass = (pass && stats.sensor_frame_sequence == 1u && stats.control_tick_count == 0u);
    write_result(out, "TIMER_FIX03", pass, "five 1 ms sensor ticks publish one complete frame sequence", stats.sensor_frame_sequence, stats.control_tick_count, stats.speed_pi_pair_calls);
    return pass;
}

static int timer_fix04_one_pid_per_sequence(FILE *out)
{
    app_context_t ctx;
    app_control_tick_stats_t stats;
    u8 i;
    int pass;

    setup_context(&ctx);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        app_control_tick_sensor_isr(&ctx, i);
    }
    app_control_tick_control_isr(&ctx, 5ul);
    app_control_tick_control_isr(&ctx, 10ul);
    stats = app_control_tick_stats();
    pass = (stats.control_tick_count == 2u &&
            stats.speed_pi_pair_calls == 1u &&
            stats.control_skipped_duplicate_frame_count == 1u);
    write_result(out, "TIMER_FIX04", pass, "same frame sequence triggers only one full PID chain", stats.control_tick_count, stats.speed_pi_pair_calls, stats.control_skipped_duplicate_frame_count);
    return pass;
}

static int timer_fix05_no_new_frame_skips_pi(FILE *out)
{
    app_context_t ctx;
    app_control_tick_stats_t stats;
    int pass;

    setup_context(&ctx);
    app_control_tick_control_isr(&ctx, 5ul);
    stats = app_control_tick_stats();
    pass = (stats.control_tick_count == 1u &&
            stats.speed_pi_pair_calls == 0u &&
            stats.control_no_new_frame_count == 1u);
    write_result(out, "TIMER_FIX05", pass, "Timer11 without a complete new frame skips speed PI", stats.control_tick_count, stats.speed_pi_pair_calls, stats.control_no_new_frame_count);
    return pass;
}

static int timer_fix06_stale_frame_safety(FILE *out)
{
    app_context_t ctx;
    app_control_tick_stats_t stats;
    u8 i;
    int pass;

    setup_context(&ctx);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        app_control_tick_sensor_isr(&ctx, i);
    }
    app_control_tick_control_isr(&ctx, (u32)SENSOR_STALE_TIMEOUT_MS + 20ul);
    stats = app_control_tick_stats();
    pass = (ctx.emag.valid == APP_FALSE &&
            ctx.emag.line_lost != APP_FALSE &&
            stats.sensor_frame_stale_count == 1u &&
            stats.control_deadline_miss_count == 1u);
    write_result(out, "TIMER_FIX06", pass, "stale frame enters line-lost safety path", ctx.emag.valid, stats.sensor_frame_stale_count, stats.control_deadline_miss_count);
    return pass;
}

static int timer_fix07_no_torn_frame(FILE *out)
{
    emag_frame_t frame;
    u8 i;
    int pass;

    bsp_emag_init();
    bsp_emag_host_set_raw(101u, 102u, 103u, 104u, 105u, APP_TRUE);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        bsp_emag_sensor_tick(i);
    }
    bsp_emag_host_begin_read_for_test();
    bsp_emag_host_set_raw(201u, 202u, 203u, 204u, 205u, APP_TRUE);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        bsp_emag_sensor_tick((u32)(10u + i));
    }
    pass = (bsp_emag_host_copy_locked_frame_for_test(&frame) != APP_FALSE &&
            frame.sequence == 1u &&
            frame.raw[0] == 101u &&
            frame.raw[1] == 102u &&
            frame.raw[2] == 103u &&
            frame.raw[3] == 104u &&
            frame.raw[4] == 105u);
    bsp_emag_host_end_read_for_test();
    write_result(out, "TIMER_FIX07", pass, "reader lock copies one complete sequence without torn channels", frame.sequence, frame.raw[0], frame.raw[4]);
    return pass;
}

static int timer_fix08_write_avoids_front_and_reader(FILE *out)
{
    u8 front;
    u8 write;
    u8 reader;
    u8 i;
    int pass;

    bsp_emag_init();
    bsp_emag_host_set_raw(11u, 12u, 13u, 14u, 15u, APP_TRUE);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        bsp_emag_sensor_tick(i);
    }
    bsp_emag_host_begin_read_for_test();
    bsp_emag_host_set_raw(21u, 22u, 23u, 24u, 25u, APP_TRUE);
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        bsp_emag_sensor_tick((u32)(20u + i));
    }
    bsp_emag_host_debug_indices(&front, &write, &reader);
    pass = (write != front && write != reader && front != reader);
    bsp_emag_host_end_read_for_test();
    write_result(out, "TIMER_FIX08", pass, "triple buffer write index avoids front and reader", front, write, reader);
    return pass;
}

static int timer_fix09_scope_default_noop(FILE *out)
{
    int pass;

    bsp_timing_scope_sensor_enter();
    bsp_timing_scope_sensor_exit();
    bsp_timing_scope_control_enter();
    bsp_timing_scope_control_exit();
    pass = (BSP_TIMING_SCOPE_ENABLE == 0 &&
            bsp_timing_scope_debug_transition_count() == 0u);
    write_result(out, "TIMER_FIX09", pass, "timing scope no-op when disabled", BSP_TIMING_SCOPE_ENABLE, bsp_timing_scope_debug_transition_count(), 0);
    return pass;
}

static int element01_baseline_no_burst(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(0u, 0u, LINE_QUALITY_MIN, 0u, 0u, LINE_QUALITY_MIN, APP_TRUE);
    feature = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (feature.active_element_count <= ELEMENT_BURST_BASELINE_MAX_ACTIVE &&
            feature.element_burst == APP_FALSE);
    write_result(out, "ELEMENT01", pass, "normal baseline active count does not burst", feature.active_element_count, feature.baseline_element_count, feature.element_burst);
    return pass;
}

static int element02_burst_confirm(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(220u, 220u, 220u, 0u, 0u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    feature = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (feature.element_burst != APP_FALSE &&
            feature.element_burst_rising_edge != APP_FALSE &&
            track_features_special_candidate() == TRACK_SPECIAL_ELEMENT_CANDIDATE);
    write_result(out, "ELEMENT02", pass, "active element count burst confirms special element", feature.active_element_count, feature.element_count_rise, track_features_special_candidate());
    return pass;
}

static int element03_rising_edge_once(FILE *out)
{
    track_element_feature_t first;
    track_element_feature_t second;
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(220u, 220u, 220u, 0u, 0u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    first = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    second = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (first.element_burst_rising_edge != APP_FALSE &&
            second.element_burst != APP_FALSE &&
            second.element_burst_rising_edge == APP_FALSE);
    write_result(out, "ELEMENT03", pass, "burst rising edge is one-shot", first.element_burst_rising_edge, second.element_burst, second.element_burst_rising_edge);
    return pass;
}

static int element04_low_quality_blocks(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(220u, 220u, 220u, 0u, 0u, (u16)(ELEMENT_BURST_MIN_QUALITY - 1u), APP_TRUE);
    feature = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (feature.element_burst == APP_FALSE &&
            track_features_special_candidate() == TRACK_SPECIAL_NONE);
    write_result(out, "ELEMENT04", pass, "low quality blocks element burst", feature.element_burst, sample.line_quality, track_features_special_candidate());
    return pass;
}

static int element05_release_rearms(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t burst;
    emag_sample_t normal;
    int pass;

    track_features_reset();
    burst = make_emag(220u, 220u, 220u, 0u, 0u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    normal = make_emag(0u, 0u, LINE_QUALITY_MIN, 0u, 0u, LINE_QUALITY_MIN, APP_TRUE);
    track_features_update_elements(&burst, ELEMENT_BURST_CONFIRM_MS);
    feature = track_features_update_elements(&normal, ELEMENT_BURST_RELEASE_MS);
    pass = (feature.element_burst == APP_FALSE &&
            feature.element_burst_armed != APP_FALSE &&
            track_features_special_candidate() == TRACK_SPECIAL_NONE);
    write_result(out, "ELEMENT05", pass, "normal line releases and rearms detector", feature.element_burst, feature.element_burst_armed, track_features_special_candidate());
    return pass;
}

static int element06_unknown_direction_generic(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(220u, 220u, 220u, 0u, 0u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    feature = track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (feature.element_burst != APP_FALSE &&
            ELEMENT_SPECIAL_DIRECTION_CONFIGURED == 0 &&
            track_features_special_candidate() == TRACK_SPECIAL_ELEMENT_CANDIDATE);
    write_result(out, "ELEMENT06", pass, "default unknown direction keeps generic candidate", ELEMENT_SPECIAL_DIRECTION_CONFIGURED, feature.element_burst, track_features_special_candidate());
    return pass;
}

static int element07_no_right_or_ring_guess(FILE *out)
{
    emag_sample_t sample;
    int pass;

    track_features_reset();
    sample = make_emag(0u, 0u, 220u, 220u, 220u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    track_features_update_elements(&sample, ELEMENT_BURST_CONFIRM_MS);
    pass = (track_features_detect_crossing(&sample) == APP_FALSE &&
            track_features_detect_hex_loop(&sample) == APP_FALSE);
    write_result(out, "ELEMENT07", pass, "unknown direction does not guess right-angle or ring", track_features_detect_crossing(&sample), track_features_detect_hex_loop(&sample), track_features_special_candidate());
    return pass;
}

static int element08_invalid_clears(FILE *out)
{
    track_element_feature_t feature;
    emag_sample_t burst;
    emag_sample_t invalid;
    int pass;

    track_features_reset();
    burst = make_emag(220u, 220u, 220u, 0u, 0u, ELEMENT_BURST_MIN_QUALITY, APP_TRUE);
    invalid = make_emag(0u, 0u, 0u, 0u, 0u, 0u, APP_FALSE);
    track_features_update_elements(&burst, ELEMENT_BURST_CONFIRM_MS);
    feature = track_features_update_elements(&invalid, ELEMENT_BURST_CONFIRM_MS);
    pass = (feature.element_burst == APP_FALSE &&
            track_features_special_candidate() == TRACK_SPECIAL_NONE);
    write_result(out, "ELEMENT08", pass, "invalid electromagnetic sample clears candidate", feature.element_burst, invalid.valid, track_features_special_candidate());
    return pass;
}

static int fan01_defaults_locked(FILE *out)
{
    int pass;

    pass = (FAN_ESC_PHYSICAL_OUTPUT_ENABLE == 0 &&
            WALL_RUN_ENABLE == 0 &&
            SUCTION_HW_VERIFIED == 0 &&
            FAN_BENCH_TEST_ENABLE == 0);
    write_result(out, "FAN01", pass, "fan and wall safety defaults remain locked", FAN_ESC_PHYSICAL_OUTPUT_ENABLE, WALL_RUN_ENABLE, FAN_BENCH_TEST_ENABLE);
    return pass;
}

static int fan02_physical_output_zero(FILE *out)
{
    ctrl_adhesion_state_t state;
    fan_esc_command_t command;
    int pass;

    bsp_fan_esc_init();
    ctrl_adhesion_init(&state);
    ctrl_adhesion_update(&state,
                         TRACK_WALL_FAN_PRECHARGE,
                         0u,
                         TASK_CONTROL_PERIOD_MS,
                         &command);
    bsp_fan_esc_apply(&command);
    pass = (command.request_us == FAN_PRECHARGE_US &&
            command.output_us == 0u &&
            bsp_fan_esc_last_output_us() == 0u);
    write_result(out, "FAN02", pass, "logical fan request keeps physical output zero", command.request_us, command.output_us, bsp_fan_esc_last_output_us());
    return pass;
}

static int fan03_force_off(FILE *out)
{
    fan_esc_command_t command;
    int pass;

    bsp_fan_esc_init();
    command.state = FAN_ESC_BOOST;
    command.request_us = FAN_BOOST_US;
    command.output_us = FAN_BOOST_US;
    command.mapped = APP_TRUE;
    command.physical_enabled = APP_TRUE;
    command.bench_verified = APP_TRUE;
    bsp_fan_esc_apply(&command);
    bsp_fan_esc_force_off();
    pass = (bsp_fan_esc_last_output_us() == 0u &&
            bsp_fan_esc_last_applied_us() == 0u &&
            bsp_fan_esc_is_physical_active() == APP_FALSE);
    write_result(out, "FAN03", pass, "force off clears fan output", bsp_fan_esc_last_output_us(), bsp_fan_esc_last_applied_us(), bsp_fan_esc_is_physical_active());
    return pass;
}

static int fan04_bench_default_disabled(FILE *out)
{
    int pass;

    pass = (FAN_BENCH_TEST_ENABLE == 0);
    write_result(out, "FAN04", pass, "fan bench mode is compile-time disabled by default", FAN_BENCH_TEST_ENABLE, FAN_ESC_PHYSICAL_OUTPUT_ENABLE, WALL_RUN_ENABLE);
    return pass;
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed;
    int total;

    if (argc != 2) {
        fputs("usage: timing_element_fan_runner <summary.csv>\n", stderr);
        return 2;
    }
    out = fopen(argv[1], "w");
    if (out == 0) {
        fputs("failed to open summary csv\n", stderr);
        return 2;
    }
    fputs("scenario,status,detail,value1,value2,value3\n", out);

    passed = 0;
    total = 0;

#define RUN_SCENARIO(fn) do { total++; if ((fn)(out)) passed++; } while (0)
    RUN_SCENARIO(timer_fix01_frequency_relationship);
    RUN_SCENARIO(timer_fix02_pid_not_above_frame);
    RUN_SCENARIO(timer_fix03_five_ticks_one_sequence);
    RUN_SCENARIO(timer_fix04_one_pid_per_sequence);
    RUN_SCENARIO(timer_fix05_no_new_frame_skips_pi);
    RUN_SCENARIO(timer_fix06_stale_frame_safety);
    RUN_SCENARIO(timer_fix07_no_torn_frame);
    RUN_SCENARIO(timer_fix08_write_avoids_front_and_reader);
    RUN_SCENARIO(timer_fix09_scope_default_noop);
    RUN_SCENARIO(element01_baseline_no_burst);
    RUN_SCENARIO(element02_burst_confirm);
    RUN_SCENARIO(element03_rising_edge_once);
    RUN_SCENARIO(element04_low_quality_blocks);
    RUN_SCENARIO(element05_release_rearms);
    RUN_SCENARIO(element06_unknown_direction_generic);
    RUN_SCENARIO(element07_no_right_or_ring_guess);
    RUN_SCENARIO(element08_invalid_clears);
    RUN_SCENARIO(fan01_defaults_locked);
    RUN_SCENARIO(fan02_physical_output_zero);
    RUN_SCENARIO(fan03_force_off);
    RUN_SCENARIO(fan04_bench_default_disabled);
#undef RUN_SCENARIO

    fclose(out);
    printf("timing/element/fan scenarios: %d/%d passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}

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
#include "../../BSP/bsp_steering.h"
#include "../../BSP/bsp_suction.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"
#include "../../Track/track_full_course_profile.h"
#include "../../Track/track_route_event.h"

typedef struct {
    app_state_t final_state;
    u32 segment_mask;
    u16 max_fan_request_us;
    u16 max_fan_output_us;
    s16 max_turn_abs;
    s16 max_target_delta_abs;
    s16 max_drive_delta_abs;
    u8 saw_precharge;
    u8 saw_wall;
    u16 steering_apply_count;
} route_observation_t;

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

static s16 abs_s16(s16 value)
{
    if (value < 0) {
        return (s16)-value;
    }
    return value;
}

static u32 segment_bit(track_course_segment_t segment)
{
    return (u32)(1ul << (u8)segment);
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
    bsp_fan_esc_init();
    app_state_machine_init(ctx);
    app_scheduler_init();
}

static void clear_input(host_sil_input_t *input, u32 t)
{
    input->time_ms = t;
    input->manual_arm = (t >= 80ul) ? APP_TRUE : APP_FALSE;
    input->suction_authorize = APP_FALSE;
    input->transition_candidate = APP_FALSE;
    input->route_event = track_route_event_none();
    input->emag_valid = APP_TRUE;
    input->line_error = 0;
    input->signal_quality = 900u;
    input->emag_norm_valid = APP_TRUE;
    input->emag_norm[0] = 0u;
    input->emag_norm[1] = 100u;
    input->emag_norm[2] = 900u;
    input->emag_norm[3] = 100u;
    input->emag_norm[4] = 0u;
    input->imu_fresh = APP_TRUE;
    input->imu_id_ok = APP_TRUE;
    input->pitch_cdeg = 0;
    input->pitch_rate_cdeg_s = 0;
    input->encoder_valid = APP_TRUE;
    input->left_count = (s32)t;
    input->right_count = (s32)t;
    input->left_speed_mm_s = 90;
    input->right_speed_mm_s = 90;
    input->power_ok = APP_TRUE;
    input->kill_switch = APP_FALSE;
    input->control_period_ok = APP_TRUE;
    input->force_app_state = -1;
}

static void set_normal_curve_line(host_sil_input_t *input)
{
    input->emag_norm[0] = 0u;
    input->emag_norm[1] = 100u;
    input->emag_norm[2] = 700u;
    input->emag_norm[3] = 400u;
    input->emag_norm[4] = 0u;
}

static void set_sharp_curve_line(host_sil_input_t *input)
{
    input->emag_norm[0] = 0u;
    input->emag_norm[1] = 0u;
    input->emag_norm[2] = 200u;
    input->emag_norm[3] = 300u;
    input->emag_norm[4] = 700u;
}

static s16 full_route_pitch_for(u32 t)
{
    if (t < 1000ul) {
        return 0;
    }
    if (t < 1700ul) {
        return 5200;
    }
    if (t < 1950ul) {
        return -3200;
    }
    if (t < 2250ul) {
        return 5200;
    }
    return 0;
}

static void set_full_route_input(host_sil_input_t *input, u32 t, u8 finish_after_recovery)
{
    clear_input(input, t);
    if (t >= 220ul && t < 320ul) {
        set_normal_curve_line(input);
    } else if (t >= 320ul && t < 430ul) {
        set_sharp_curve_line(input);
    } else if (t >= 430ul && t < 500ul) {
        set_sharp_curve_line(input);
        input->route_event.omega_event = APP_TRUE;
    } else if (t >= 500ul && t < 570ul) {
        input->route_event.hex_loop_event = APP_TRUE;
    } else if (t >= 570ul && t < 630ul) {
        input->route_event.crossing_event = APP_TRUE;
    }

    if (t >= 650ul && t < 1500ul) {
        input->route_event.wall_approach_event = APP_TRUE;
    }
    if (finish_after_recovery != APP_FALSE && t >= 2450ul) {
        input->route_event.finish_event = APP_TRUE;
    }
    input->pitch_cdeg = full_route_pitch_for(t);
}

static route_observation_t run_full_route(u8 finish_after_recovery)
{
    app_context_t ctx;
    host_sil_input_t input;
    route_observation_t obs;
    s16 target_delta;
    s16 drive_delta;
    u32 t;

    init_host_system(&ctx);
    obs.final_state = APP_STATE_BOOT;
    obs.segment_mask = 0ul;
    obs.max_fan_request_us = 0u;
    obs.max_fan_output_us = 0u;
    obs.max_turn_abs = 0;
    obs.max_target_delta_abs = 0;
    obs.max_drive_delta_abs = 0;
    obs.saw_precharge = APP_FALSE;
    obs.saw_wall = APP_FALSE;
    obs.steering_apply_count = 0u;

    for (t = 1ul; t <= 3900ul; t++) {
        set_full_route_input(&input, t, finish_after_recovery);
        host_bsp_set_input(&input);
        bsp_timebase_on_tick_1ms();
        app_scheduler_run_due(&ctx, bsp_timebase_now_ms());

        obs.segment_mask |= segment_bit(ctx.course_segment);
        if (ctx.fan_cmd.request_us > obs.max_fan_request_us) {
            obs.max_fan_request_us = ctx.fan_cmd.request_us;
        }
        if (bsp_fan_esc_last_output_us() > obs.max_fan_output_us) {
            obs.max_fan_output_us = bsp_fan_esc_last_output_us();
        }
        if (abs_s16(ctx.turn_delta_mm_s) > obs.max_turn_abs) {
            obs.max_turn_abs = abs_s16(ctx.turn_delta_mm_s);
        }
        target_delta = (s16)(ctx.left_speed_target_mm_s - ctx.right_speed_target_mm_s);
        if (abs_s16(target_delta) > obs.max_target_delta_abs) {
            obs.max_target_delta_abs = abs_s16(target_delta);
        }
        drive_delta = (s16)(ctx.left_drive_command_native - ctx.right_drive_command_native);
        if (abs_s16(drive_delta) > obs.max_drive_delta_abs) {
            obs.max_drive_delta_abs = abs_s16(drive_delta);
        }
        if (ctx.fan_cmd.state == FAN_ESC_PRECHARGE) {
            obs.saw_precharge = APP_TRUE;
        }
        if (ctx.app_state == APP_STATE_WALL_TRACK ||
            ctx.app_state == APP_STATE_CYLINDER_TRACK ||
            ctx.app_state == APP_STATE_TRANSITION_DOWN) {
            obs.saw_wall = APP_TRUE;
        }
    }

    obs.final_state = ctx.app_state;
    obs.steering_apply_count = host_bsp_steering_apply_count();
    return obs;
}

static route_observation_t run_no_wall_route(void)
{
    app_context_t ctx;
    host_sil_input_t input;
    route_observation_t obs;
    u32 t;

    init_host_system(&ctx);
    obs.final_state = APP_STATE_BOOT;
    obs.segment_mask = 0ul;
    obs.max_fan_request_us = 0u;
    obs.max_fan_output_us = 0u;
    obs.max_turn_abs = 0;
    obs.max_target_delta_abs = 0;
    obs.max_drive_delta_abs = 0;
    obs.saw_precharge = APP_FALSE;
    obs.saw_wall = APP_FALSE;
    obs.steering_apply_count = 0u;

    for (t = 1ul; t <= 1800ul; t++) {
        clear_input(&input, t);
        if (t >= 220ul && t < 600ul) {
            set_normal_curve_line(&input);
        }
        input.pitch_cdeg = (t >= 900ul) ? 5200 : 0;
        host_bsp_set_input(&input);
        bsp_timebase_on_tick_1ms();
        app_scheduler_run_due(&ctx, bsp_timebase_now_ms());
        obs.segment_mask |= segment_bit(ctx.course_segment);
        if (ctx.fan_cmd.request_us > obs.max_fan_request_us) {
            obs.max_fan_request_us = ctx.fan_cmd.request_us;
        }
        if (bsp_fan_esc_last_output_us() > obs.max_fan_output_us) {
            obs.max_fan_output_us = bsp_fan_esc_last_output_us();
        }
        if (ctx.fan_cmd.state == FAN_ESC_PRECHARGE) {
            obs.saw_precharge = APP_TRUE;
        }
        if (ctx.app_state == APP_STATE_WALL_TRACK ||
            ctx.app_state == APP_STATE_CYLINDER_TRACK ||
            ctx.app_state == APP_STATE_TRANSITION_DOWN) {
            obs.saw_wall = APP_TRUE;
        }
    }
    obs.final_state = ctx.app_state;
    obs.steering_apply_count = host_bsp_steering_apply_count();
    return obs;
}

static int f01_complete_route(FILE *out)
{
    route_observation_t obs;
    u32 required;
    int pass;

    obs = run_full_route(APP_TRUE);
    required = segment_bit(TRACK_COURSE_START) |
               segment_bit(TRACK_COURSE_GROUND_STRAIGHT) |
               segment_bit(TRACK_COURSE_NORMAL_CURVE) |
               segment_bit(TRACK_COURSE_SHARP_CURVE) |
               segment_bit(TRACK_COURSE_CROSSING) |
               segment_bit(TRACK_COURSE_OMEGA) |
               segment_bit(TRACK_COURSE_HEX_LOOP) |
               segment_bit(TRACK_COURSE_WALL_APPROACH) |
               segment_bit(TRACK_COURSE_FAN_PRECHARGE) |
               segment_bit(TRACK_COURSE_TRANSITION_UP) |
               segment_bit(TRACK_COURSE_WALL_TRACK) |
               segment_bit(TRACK_COURSE_CYLINDER_TRACK) |
               segment_bit(TRACK_COURSE_TRANSITION_DOWN) |
               segment_bit(TRACK_COURSE_GROUND_RECOVERY) |
               segment_bit(TRACK_COURSE_FINISH);
    pass = ((obs.segment_mask & required) == required &&
            obs.final_state == APP_STATE_FINISHED);
    write_result(out, "F01", pass, pass ? "complete full-course segment path reached" : "full-course segment path missing", (int)obs.segment_mask, (int)required, (int)obs.final_state);
    return pass;
}

static int f02_physical_fan_stays_zero(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_full_route(APP_TRUE);
    pass = (obs.max_fan_request_us > FAN_ESC_MIN_US &&
            obs.max_fan_output_us == 0u);
    write_result(out, "F02", pass, pass ? "logical fan changes while physical P2.2 remains zero" : "physical fan output became nonzero", obs.max_fan_request_us, obs.max_fan_output_us, FAN_ESC_PHYSICAL_OUTPUT_ENABLE);
    return pass;
}

static int f03_no_wall_event_no_precharge(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_no_wall_route();
    pass = (obs.saw_precharge == APP_FALSE &&
            obs.saw_wall == APP_FALSE &&
            obs.final_state == APP_STATE_GROUND_TRACK);
    write_result(out, "F03", pass, pass ? "no WALL_APPROACH event keeps wall process idle" : "wall process started without route event", obs.saw_precharge, obs.saw_wall, obs.final_state);
    return pass;
}

static int f04_recovery_without_finish_returns_ground(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_full_route(APP_FALSE);
    pass = (obs.final_state == APP_STATE_GROUND_TRACK &&
            (obs.segment_mask & segment_bit(TRACK_COURSE_GROUND_RECOVERY)) != 0ul &&
            (obs.segment_mask & segment_bit(TRACK_COURSE_FINISH)) == 0ul);
    write_result(out, "F04", pass, pass ? "ground recovery without FINISH returns to ground track" : "ground recovery ended incorrectly", (int)obs.final_state, (int)obs.segment_mask, 0);
    return pass;
}

static int f05_recovery_with_finish_ends(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_full_route(APP_TRUE);
    pass = (obs.final_state == APP_STATE_FINISHED &&
            (obs.segment_mask & segment_bit(TRACK_COURSE_FINISH)) != 0ul);
    write_result(out, "F05", pass, pass ? "finish event after recovery enters FINISHED" : "finish event did not finish", (int)obs.final_state, (int)obs.segment_mask, 0);
    return pass;
}

static int f06_fault_outputs_zero(FILE *out)
{
    app_context_t ctx;
    app_state_t states[5];
    u8 i;
    int pass;

    states[0] = APP_STATE_GROUND_FAULT;
    states[1] = APP_STATE_SUCTION_LOCKOUT;
    states[2] = APP_STATE_WALL_FAILSAFE_HOLD;
    states[3] = APP_STATE_HARD_FAULT;
    states[4] = APP_STATE_FINISHED;
    pass = 1;

    for (i = 0u; i < 5u; i++) {
        app_state_machine_init(&ctx);
        ctx.app_state = states[i];
        ctx.manual_arm = APP_TRUE;
        ctx.left_drive_command_native = 220;
        ctx.right_drive_command_native = -160;
        ctx.fan_cmd.state = FAN_ESC_HOLD;
        ctx.fan_cmd.request_us = FAN_HOLD_US;
        ctx.fan_cmd.output_us = FAN_HOLD_US;
        app_output_arbitrate(&ctx);
        if (ctx.left_drive_command_native != 0 ||
            ctx.right_drive_command_native != 0 ||
            ctx.fan_cmd.output_us != 0u) {
            pass = 0;
        }
    }
    write_result(out, "F06", pass, pass ? "fault and finish states zero both motors and physical fan" : "a fault path left output active", ctx.left_drive_command_native, ctx.right_drive_command_native, ctx.fan_cmd.output_us);
    return pass;
}

static int f07_fuzzy_changes_differential_targets(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_full_route(APP_TRUE);
    pass = (obs.max_turn_abs > 0 &&
            obs.max_target_delta_abs > 0);
    write_result(out, "F07", pass, pass ? "turn controller changes left/right speed targets" : "turn controller did not affect differential targets", obs.max_turn_abs, obs.max_target_delta_abs, 0);
    return pass;
}

static int f08_speed_pi_not_overwritten(FILE *out)
{
    app_context_t ctx;
    host_sil_input_t input;
    s16 drive_delta;
    u32 t;
    int pass;

    init_host_system(&ctx);
    for (t = 1ul; t <= 900ul; t++) {
        clear_input(&input, t);
        input.left_speed_mm_s = 0;
        input.right_speed_mm_s = 160;
        host_bsp_set_input(&input);
        bsp_timebase_on_tick_1ms();
        app_scheduler_run_due(&ctx, bsp_timebase_now_ms());
    }
    drive_delta = (s16)(ctx.left_drive_command_native - ctx.right_drive_command_native);
    pass = (ctx.app_state == APP_STATE_GROUND_TRACK &&
            drive_delta != 0 &&
            ctx.drive_command_native == (s16)(((s32)ctx.left_drive_command_native + (s32)ctx.right_drive_command_native) / 2l));
    write_result(out, "F08", pass, pass ? "left/right speed PI outputs remain independent" : "speed PI outputs were overwritten or inactive", ctx.left_drive_command_native, ctx.right_drive_command_native, ctx.drive_command_native);
    return pass;
}

static int f09_no_legacy_steering_runtime(FILE *out)
{
    route_observation_t obs;
    int pass;

    obs = run_full_route(APP_TRUE);
    pass = (obs.steering_apply_count == 0u);
    write_result(out, "F09", pass, pass ? "scheduler did not call legacy steering BSP path" : "legacy steering BSP was called", obs.steering_apply_count, 0, 0);
    return pass;
}

int main(int argc, char **argv)
{
    FILE *out;
    int passed;
    int total;

    if (argc != 2) {
        fputs("usage: final_full_course_runner <summary.csv>\n", stderr);
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
    passed += f01_complete_route(out); total++;
    passed += f02_physical_fan_stays_zero(out); total++;
    passed += f03_no_wall_event_no_precharge(out); total++;
    passed += f04_recovery_without_finish_returns_ground(out); total++;
    passed += f05_recovery_with_finish_ends(out); total++;
    passed += f06_fault_outputs_zero(out); total++;
    passed += f07_fuzzy_changes_differential_targets(out); total++;
    passed += f08_speed_pi_not_overwritten(out); total++;
    passed += f09_no_legacy_steering_runtime(out); total++;

    fclose(out);
    if (passed != total) {
        return 1;
    }
    return 0;
}

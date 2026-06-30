/*
 * Real-time control ownership:
 * - sensor tick only advances the five-channel electromagnetic frame.
 * - control tick consumes a complete frame and owns all real-time PID/output work.
 * - scheduler/main remain low-frequency orchestration and must not call PID again.
 */
#include "app_control_tick.h"
#include "app_build_profile.h"
#include "app_config.h"
#include "app_output_arbitration.h"
#include "app_state_machine.h"
#include "app_safety.h"
#include "../BSP/bsp_drive.h"
#include "../BSP/bsp_fan_esc.h"
#include "../BSP/board_map.h"
#include "../BSP/bsp_emag.h"
#include "../BSP/bsp_imu.h"
#include "../BSP/bsp_encoder.h"
#include "../BSP/bsp_power.h"
#include "../Control/ctrl_adhesion.h"
#include "../Control/ctrl_attitude.h"
#include "../Control/ctrl_differential_drive.h"
#include "../Control/ctrl_fuzzy_turn.h"
#include "../Control/ctrl_line.h"
#include "../Control/ctrl_profile.h"
#include "../Control/ctrl_speed.h"
#include "../Track/track_features.h"
#include "../Track/track_full_course_profile.h"
#include "../Track/track_route_event.h"
#include "../Track/track_route_profile.h"
#include "../Track/track_state_machine.h"
#include "../Track/track_strategy.h"
#include "../Track/track_surface_state.h"
#include "../Track/track_wall_logic.h"
#ifdef HOST_SIL
#include "../sim/host/host_bsp.h"
#endif

static track_mode_state_t g_track_mode_state;
static ctrl_fuzzy_turn_state_t g_fuzzy_turn_state;
static line_filter_state_t g_line_filter_state;
static speed_pi_state_t g_left_speed_pi;
static speed_pi_state_t g_right_speed_pi;
static track_wall_logic_t g_wall_logic;
static track_full_course_profile_t g_full_course_profile;
static ctrl_adhesion_state_t g_adhesion_state;
static u16 g_line_lost_elapsed_ms;
static app_control_tick_stats_t g_stats;
static app_context_t *g_bound_context;
static u16 g_last_consumed_frame_sequence;

#if defined(HOST_SIL) && !defined(HOST_SIL_REAL_EMAG_SCAN)
static emag_sample_t g_host_sensor_sample;
static u32 g_host_sensor_timestamp_ms;
static u8 g_host_sensor_complete;
static u16 g_host_sensor_sequence;
#endif

#if FAN_BENCH_TEST_ENABLE != 0
static u16 g_fan_bench_elapsed_ms;
#endif

static u16 control_dt_ms(void)
{
    if (CONTROL_PID_HZ == 0u) {
        return TASK_CONTROL_PERIOD_MS;
    }
    return (u16)(1000u / CONTROL_PID_HZ);
}

static void make_stale_emag(emag_sample_t *sample)
{
    u8 i;

    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        sample->raw[i] = 0u;
        sample->filtered[i] = 0u;
        sample->norm[i] = 0u;
    }
    sample->line_error = 0;
    sample->line_quality = 0u;
    sample->signal_quality = 0u;
    sample->channel_count = EMAG_CHANNEL_COUNT;
    sample->line_lost = APP_TRUE;
    sample->valid = APP_FALSE;
}

#define CONTROL_FRAME_NONE 0u
#define CONTROL_FRAME_NEW 1u
#define CONTROL_FRAME_DUPLICATE 2u
#define CONTROL_FRAME_STALE 3u

static void update_sensor_health(app_context_t *ctx, u32 now_ms)
{
    u32 attitude_age_ms;
    power_sample_t power;

    ctx->health.emag_ok = (ctx->emag.valid &&
                           ctx->emag.line_lost == APP_FALSE &&
                           ctx->emag.line_quality >= LINE_QUALITY_MIN);
    if (ctx->attitude.fresh && now_ms >= ctx->attitude.timestamp_ms) {
        attitude_age_ms = now_ms - ctx->attitude.timestamp_ms;
        ctx->health.imu_fresh = (attitude_age_ms <= IMU_SENSOR_STALE_TIMEOUT_MS);
    } else {
        ctx->health.imu_fresh = APP_FALSE;
    }
    ctx->health.encoder_ok = ctx->encoder.valid;
    power = bsp_power_last_sample();
    ctx->health.power_ok = power.power_good;
#ifdef HOST_SIL
    ctx->health.control_period_ok = host_bsp_control_period_ok();
    ctx->kill_switch = host_bsp_kill_switch();
#else
    ctx->health.control_period_ok = APP_TRUE;
    ctx->kill_switch = APP_FALSE;
#endif
    ctx->health.suction_feedback_ok = SUCTION_FEEDBACK_AVAILABLE;

    if (ctx->health.power_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_HARD_POWER);
    }
    if ((ctx->app_state == APP_STATE_TRANSITION_UP ||
         ctx->app_state == APP_STATE_WALL_TRACK ||
         ctx->app_state == APP_STATE_CYLINDER_TRACK ||
         ctx->app_state == APP_STATE_TRANSITION_DOWN) &&
        (ctx->health.imu_fresh == APP_FALSE || ctx->health.encoder_ok == APP_FALSE)) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
    }
    if (ctx->attitude.id_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_IMU_INVALID);
    }
    if (ctx->health.control_period_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_CONTROL_OVERRUN);
    }
}

static u8 read_transition_candidate(void)
{
#ifdef HOST_SIL
    return host_bsp_transition_candidate();
#else
    return APP_FALSE;
#endif
}

static track_route_event_t read_host_route_event(void)
{
#ifdef HOST_SIL
    return host_bsp_route_event();
#else
    return track_route_event_none();
#endif
}

static u8 selected_route_event_source(void)
{
#ifdef HOST_SIL
    return ROUTE_EVENT_SOURCE_HOST_SIL;
#else
    return ROUTE_EVENT_SOURCE_DEFAULT;
#endif
}

#ifdef HOST_SIL
static void apply_host_fault_injection(app_context_t *ctx)
{
    s16 forced_state;
    forced_state = host_bsp_force_app_state();
    if (forced_state >= 0) {
        ctx->app_state = (app_state_t)forced_state;
    }
}
#endif

static void update_line_error_terms(app_context_t *ctx)
{
    ctrl_line_filter_update(&g_line_filter_state,
                            &ctx->emag,
                            &ctx->line_error_filtered,
                            &ctx->line_error_rate);
}

static void update_track_mode(app_context_t *ctx,
                              const track_element_feature_t *element_feature)
{
    track_mode_input_t input;
    s32 speed_sum;

    input.line_error = ctx->line_error_filtered;
    input.error_rate = ctx->line_error_rate;
    input.line_quality = ctx->emag.line_quality;
    input.surface_state = ctx->surface_state;
    input.pitch_cdeg = ctx->attitude.pitch_cdeg;
    input.pitch_rate_cdeg_s = 0;
    if (ctx->encoder.speed_mm_s_valid != APP_FALSE) {
        speed_sum = (s32)ctx->encoder.left_speed_mm_s + (s32)ctx->encoder.right_speed_mm_s;
        input.speed_mm_s = (s16)(speed_sum / 2l);
    } else {
        input.speed_mm_s = 0;
    }
    input.cross_confirmed = track_features_detect_crossing(&ctx->emag);
    input.hex_confirmed = track_features_detect_hex_loop(&ctx->emag);
    input.seesaw_confirmed = track_features_detect_seesaw(&ctx->attitude);
    input.element_feature = *element_feature;
    input.special_candidate = track_features_special_candidate();
    input.dt_ms = control_dt_ms();

    ctx->track_mode = track_route_profile_select(&g_track_mode_state, &input);
    if (ctx->app_state == APP_STATE_GROUND_RECOVERY) {
        ctx->track_mode = TRACK_MODE_RECOVERY;
    } else if (ctx->app_state == APP_STATE_SEESAW_PASS) {
        ctx->track_mode = TRACK_MODE_SEESAW;
    }
}

static u16 estimate_adhesion_risk(const app_context_t *ctx)
{
    u16 risk = 0u;
    if (ctx->surface_state == SURFACE_UNKNOWN) risk = (u16)(risk + 250u);
    if (ctx->health.imu_fresh == APP_FALSE) risk = (u16)(risk + 250u);
    if (ctx->emag.line_lost != APP_FALSE || ctx->emag.line_quality < LINE_QUALITY_MIN) risk = (u16)(risk + 250u);
    if (ctx->health.encoder_ok == APP_FALSE) risk = (u16)(risk + 150u);
    if (ctx->health.power_ok == APP_FALSE) risk = (u16)(risk + 150u);
    if (risk > 1000u) risk = 1000u;
    return risk;
}

static void update_fuzzy_turn(app_context_t *ctx, u8 outputs_allowed)
{
    ctrl_fuzzy_turn_input_t input;
    s16 turn;

    input.mode = ctx->track_mode;
    input.app_state = ctx->app_state;
    input.line_error_filtered = ctx->line_error_filtered;
    input.error_rate = ctx->line_error_rate;
    input.signal_quality = ctx->emag.line_quality;
    input.dt_ms = control_dt_ms();
    input.outputs_allowed = outputs_allowed;

    turn = ctrl_fuzzy_turn_update(&g_fuzzy_turn_state, &input);
    ctx->turn_delta_mm_s = turn;
    ctx->steering_offset_us = 0;
    ctx->fuzzy_kp = g_fuzzy_turn_state.gain.kp;
    ctx->fuzzy_ki = g_fuzzy_turn_state.gain.ki;
    ctx->fuzzy_kd = g_fuzzy_turn_state.gain.kd;
}

static void reset_fuzzy_if_blocked(app_context_t *ctx)
{
    if (ctrl_fuzzy_turn_needs_reset(ctx->app_state,
                                    ctx->track_mode,
                                    ctx->emag.line_quality,
                                    app_safety_outputs_allowed(ctx)) != APP_FALSE) {
        ctrl_fuzzy_turn_reset(&g_fuzzy_turn_state);
        ctx->turn_delta_mm_s = 0;
        ctx->fuzzy_kp = g_fuzzy_turn_state.gain.kp;
        ctx->fuzzy_ki = g_fuzzy_turn_state.gain.ki;
        ctx->fuzzy_kd = g_fuzzy_turn_state.gain.kd;
    }
}

static app_state_t app_state_from_wall_output(const track_wall_output_t *wall)
{
    switch (wall->state) {
    case TRACK_WALL_WALL_APPROACH: return APP_STATE_TRANSITION_CANDIDATE;
    case TRACK_WALL_FAN_PRECHARGE:
        if (wall->state_elapsed_ms >= FAN_PRECHARGE_TIME_MS) {
            return APP_STATE_APPROACH_TRANSITION;
        }
        return APP_STATE_SUCTION_PRECHARGE;
    case TRACK_WALL_TRANSITION_UP: return APP_STATE_TRANSITION_UP;
    case TRACK_WALL_WALL_TRACK: return APP_STATE_WALL_TRACK;
    case TRACK_WALL_CYLINDER_TRACK: return APP_STATE_CYLINDER_TRACK;
    case TRACK_WALL_TRANSITION_DOWN: return APP_STATE_TRANSITION_DOWN;
    case TRACK_WALL_GROUND_RECOVERY: return APP_STATE_GROUND_RECOVERY;
    case TRACK_WALL_FAILSAFE_HOLD: return APP_STATE_WALL_FAILSAFE_HOLD;
    case TRACK_WALL_GROUND_TRACK:
    default:
        return APP_STATE_GROUND_TRACK;
    }
}

static void apply_wall_output(app_context_t *ctx, const track_wall_output_t *wall)
{
    if (wall == 0) {
        return;
    }
    ctx->wall_state = wall->state;
    if (ctx->app_state == APP_STATE_GROUND_TRACK ||
        ctx->app_state == APP_STATE_TRANSITION_CANDIDATE ||
        ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION ||
        ctx->app_state == APP_STATE_TRANSITION_UP ||
        ctx->app_state == APP_STATE_WALL_TRACK ||
        ctx->app_state == APP_STATE_CYLINDER_TRACK ||
        ctx->app_state == APP_STATE_TRANSITION_DOWN ||
        ctx->app_state == APP_STATE_GROUND_RECOVERY) {
        app_state_t mapped_state;
        mapped_state = app_state_from_wall_output(wall);
        if (ctx->app_state != mapped_state) {
            ctx->app_state = mapped_state;
            ctx->state_elapsed_ms = 0u;
        }
        if (wall->state != TRACK_WALL_GROUND_TRACK) {
            ctx->state_elapsed_ms = wall->state_elapsed_ms;
        }
        if (wall->finish_ready != APP_FALSE) {
            ctx->app_state = APP_STATE_FINISHED;
            ctx->state_elapsed_ms = 0u;
        }
    }
    ctx->track_mode = wall->track_mode;
    ctx->speed_limit_mm_s = wall->speed_limit_mm_s;
}

static s32 abs_s32_local(s32 value)
{
    if (value < 0l) {
        return -value;
    }
    return value;
}

static s32 encoder_progress_distance_mm(const encoder_sample_t *encoder)
{
    if (encoder == 0 || encoder->valid == APP_FALSE || encoder->progress_mm_valid == APP_FALSE) {
        return 0l;
    }
    return (abs_s32_local(encoder->left_distance_mm) + abs_s32_local(encoder->right_distance_mm)) / 2l;
}

static void update_full_course_segment(app_context_t *ctx,
                                       const track_route_event_t *route_event)
{
    track_full_course_input_t input;

    if (ctx == 0 || route_event == 0) {
        return;
    }
    input.route_event = *route_event;
    input.wall_state = ctx->wall_state;
    input.track_mode = ctx->track_mode;
    input.app_state = ctx->app_state;
    input.line_error_filtered = ctx->line_error_filtered;
    input.line_error_rate = ctx->line_error_rate;
    input.line_quality = ctx->emag.line_quality;
    input.progress_distance_mm = encoder_progress_distance_mm(&ctx->encoder);
    ctx->course_segment = track_full_course_profile_update(&g_full_course_profile, &input);
}

static u8 update_line_lost_guard(app_context_t *ctx)
{
    if (ctx->emag.line_lost == APP_FALSE && ctx->emag.valid != APP_FALSE) {
        g_line_lost_elapsed_ms = 0u;
        return APP_FALSE;
    }

    g_line_lost_elapsed_ms = (u16)(g_line_lost_elapsed_ms + control_dt_ms());
    ctx->track_mode = TRACK_MODE_LINE_LOST;
    if (g_line_lost_elapsed_ms >= DIFF_LINE_LOST_STOP_TIME_MS) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_LINE_LOST);
        return APP_TRUE;
    }
    return APP_FALSE;
}

static u8 wall_process_active(const track_route_event_t *route_event)
{
    if (g_wall_logic.state != TRACK_WALL_GROUND_TRACK) {
        return APP_TRUE;
    }
    if (route_event != 0 &&
        route_event->wall_approach_event != APP_FALSE &&
        APP_WALL_STATE_CAPABLE != 0) {
        return APP_TRUE;
    }
    return APP_FALSE;
}

static u8 load_latest_emag_sample(app_context_t *ctx, u32 now_ms)
{
#if defined(HOST_SIL) && !defined(HOST_SIL_REAL_EMAG_SCAN)
    if (g_host_sensor_complete == APP_FALSE ||
        (now_ms - g_host_sensor_timestamp_ms) > SENSOR_STALE_TIMEOUT_MS) {
        make_stale_emag(&ctx->emag);
        if (g_host_sensor_complete == APP_FALSE) {
            return CONTROL_FRAME_NONE;
        }
        return CONTROL_FRAME_STALE;
    }
    if (g_host_sensor_sequence == g_last_consumed_frame_sequence) {
        return CONTROL_FRAME_DUPLICATE;
    }
    ctx->emag = g_host_sensor_sample;
    g_last_consumed_frame_sequence = g_host_sensor_sequence;
    return CONTROL_FRAME_NEW;
#else
    emag_frame_t frame;
    if (bsp_emag_latest_frame(&frame) == APP_FALSE) {
        return CONTROL_FRAME_NONE;
    }
    if ((now_ms - frame.timestamp_ms) > SENSOR_STALE_TIMEOUT_MS) {
        make_stale_emag(&ctx->emag);
        return CONTROL_FRAME_STALE;
    }
    if (frame.sequence == g_last_consumed_frame_sequence) {
        return CONTROL_FRAME_DUPLICATE;
    } else {
        bsp_emag_sample_from_frame(&frame, &ctx->emag);
        g_last_consumed_frame_sequence = frame.sequence;
        return CONTROL_FRAME_NEW;
    }
#endif
}

#if FAN_BENCH_TEST_ENABLE != 0
static void run_fan_bench_mode(app_context_t *ctx)
{
    fan_esc_command_t cmd;

    ctx->left_drive_command_native = DRIVE_SAFE_ZERO;
    ctx->right_drive_command_native = DRIVE_SAFE_ZERO;
    ctx->drive_command_native = DRIVE_SAFE_ZERO;
    bsp_drive_apply_lr(DRIVE_SAFE_ZERO, DRIVE_SAFE_ZERO);

    cmd.state = FAN_ESC_ARMING;
    cmd.request_us = FAN_ESC_MIN_US;
    if (g_fan_bench_elapsed_ms >= FAN_ESC_ARM_TIME_MS) {
        cmd.state = FAN_ESC_HOLD;
        cmd.request_us = FAN_BENCH_PULSE_US;
    }
    if (g_fan_bench_elapsed_ms >= FAN_BENCH_DURATION_MS ||
        ctx->faults != FAULT_NONE ||
        ctx->kill_switch != APP_FALSE) {
        bsp_fan_esc_force_off();
        return;
    }
    cmd.output_us = cmd.request_us;
    cmd.mapped = BOARD_FAN_ESC_SIGNAL_MAPPED;
    cmd.physical_enabled = FAN_ESC_PHYSICAL_OUTPUT_ENABLE;
    cmd.bench_verified = BOARD_FAN_ESC_BENCH_VERIFIED;
    ctx->fan_cmd = cmd;
    bsp_fan_esc_apply(&ctx->fan_cmd);
    g_fan_bench_elapsed_ms = (u16)(g_fan_bench_elapsed_ms + control_dt_ms());
}
#endif

void app_control_tick_init(void)
{
    ctrl_line_filter_init(&g_line_filter_state);
    ctrl_speed_pi_init(&g_left_speed_pi);
    ctrl_speed_pi_init(&g_right_speed_pi);
    track_route_profile_init(&g_track_mode_state);
    ctrl_fuzzy_turn_init(&g_fuzzy_turn_state);
    track_wall_logic_init(&g_wall_logic);
    track_full_course_profile_init(&g_full_course_profile);
    ctrl_adhesion_init(&g_adhesion_state);
    track_features_reset();
    g_line_lost_elapsed_ms = 0u;
    g_last_consumed_frame_sequence = 0u;
    g_stats.sensor_frame_sequence = 0u;
    g_stats.control_tick_count = 0u;
    g_stats.speed_pi_pair_calls = 0u;
    g_stats.sensor_frame_stale_count = 0u;
    g_stats.control_no_new_frame_count = 0u;
    g_stats.control_skipped_duplicate_frame_count = 0u;
    g_stats.control_deadline_miss_count = 0u;
    g_stats.last_sensor_frame_complete = APP_FALSE;
    g_stats.last_control_used_adc = APP_FALSE;
#if defined(HOST_SIL) && !defined(HOST_SIL_REAL_EMAG_SCAN)
    make_stale_emag(&g_host_sensor_sample);
    g_host_sensor_timestamp_ms = 0ul;
    g_host_sensor_complete = APP_FALSE;
    g_host_sensor_sequence = 0u;
#endif
#if FAN_BENCH_TEST_ENABLE != 0
    g_fan_bench_elapsed_ms = 0u;
#endif
}

void app_control_tick_bind_context(app_context_t *ctx)
{
    g_bound_context = ctx;
}

app_context_t *app_control_tick_bound_context(void)
{
    return g_bound_context;
}

void app_control_tick_sensor_isr(app_context_t *ctx, u32 now_ms)
{
#if !defined(HOST_SIL) || defined(HOST_SIL_REAL_EMAG_SCAN)
    emag_frame_t frame;
#endif

    if (ctx == 0) {
        return;
    }
#if defined(HOST_SIL) && !defined(HOST_SIL_REAL_EMAG_SCAN)
    bsp_emag_read(&g_host_sensor_sample);
    g_host_sensor_timestamp_ms = now_ms;
    g_host_sensor_complete = APP_TRUE;
    g_host_sensor_sequence++;
    g_stats.sensor_frame_sequence = g_host_sensor_sequence;
    g_stats.last_sensor_frame_complete = APP_TRUE;
#else
    bsp_emag_sensor_tick(now_ms);
    if (bsp_emag_latest_frame(&frame) != APP_FALSE) {
        g_stats.sensor_frame_sequence = frame.sequence;
        g_stats.last_sensor_frame_complete = APP_TRUE;
    } else {
        g_stats.last_sensor_frame_complete = APP_FALSE;
    }
#endif
}

void app_control_tick_control_isr(app_context_t *ctx, u32 now_ms)
{
    s16 target_speed_mm_s;
    differential_drive_output_t diff_output;
    speed_pi_output_t speed_output;
    u8 feature_transition;
    u8 outputs_allowed;
    u8 line_lost_stop;
    track_route_event_t route_event;
    track_route_event_t host_route_event;
    track_wall_input_t wall_input;
    track_wall_output_t wall_output;
    track_element_feature_t element_feature;
    u16 dt_ms;
    u8 frame_status;

    if (ctx == 0) {
        return;
    }

    dt_ms = control_dt_ms();
    g_stats.control_tick_count++;
    g_stats.last_control_used_adc = APP_FALSE;

    frame_status = load_latest_emag_sample(ctx, now_ms);
    if (frame_status == CONTROL_FRAME_NONE) {
        g_stats.control_no_new_frame_count++;
        return;
    }
    if (frame_status == CONTROL_FRAME_DUPLICATE) {
        g_stats.control_skipped_duplicate_frame_count++;
        return;
    }
    if (frame_status == CONTROL_FRAME_STALE) {
        g_stats.sensor_frame_stale_count++;
        g_stats.control_deadline_miss_count++;
    }
    ctx->encoder = bsp_encoder_read();
    ctx->attitude = bsp_imu_read();

#if FAN_BENCH_TEST_ENABLE != 0
    run_fan_bench_mode(ctx);
    return;
#endif

    ctx->emag = ctrl_line_update(ctx->emag);
    update_line_error_terms(ctx);
    element_feature = track_features_update_elements(&ctx->emag, dt_ms);
    ctx->attitude = ctrl_attitude_update(ctx->attitude, dt_ms);
    ctx->transition_candidate = read_transition_candidate();
    host_route_event = read_host_route_event();
    route_event = track_route_event_select(selected_route_event_source(),
                                           &host_route_event,
                                           &ctx->encoder);
    if (ctx->transition_candidate != APP_FALSE) {
        route_event.wall_approach_event = APP_TRUE;
    }
    update_sensor_health(ctx, now_ms);
    ctx->surface_state = track_surface_state_update(ctx->surface_state, &ctx->attitude);
    feature_transition = track_features_detect_transition(&ctx->emag);
    ctx->surface_state = track_state_machine_step(ctx->surface_state, feature_transition);
#ifdef HOST_SIL
    apply_host_fault_injection(ctx);
#endif
    if (wall_process_active(&route_event) == APP_FALSE ||
        ctx->app_state < APP_STATE_GROUND_TRACK ||
        ctx->app_state == APP_STATE_GROUND_FAULT ||
        ctx->app_state == APP_STATE_HARD_FAULT ||
        ctx->app_state == APP_STATE_FINISHED) {
        app_state_machine_step(ctx, dt_ms);
    }
    update_sensor_health(ctx, now_ms);
    ctx->adhesion_risk = estimate_adhesion_risk(ctx);
    wall_input.route_event = route_event;
    wall_input.attitude = ctx->attitude;
    wall_input.line_lost = ctx->emag.line_lost;
    wall_input.adhesion_risk = ctx->adhesion_risk;
    wall_input.dt_ms = dt_ms;
    wall_output = track_wall_logic_update(&g_wall_logic, &wall_input);
    update_track_mode(ctx, &element_feature);
    apply_wall_output(ctx, &wall_output);
    update_full_course_segment(ctx, &route_event);

    if (ctx->speed_limit_mm_s <= 0 ||
        ctx->speed_limit_mm_s > ctrl_profile_speed_limit(ctx->surface_state, ctx->adhesion_risk)) {
        ctx->speed_limit_mm_s = ctrl_profile_speed_limit(ctx->surface_state, ctx->adhesion_risk);
    }
    target_speed_mm_s = track_strategy_target_speed_for_mode(ctx->track_mode);
    if (target_speed_mm_s > ctx->speed_limit_mm_s) {
        target_speed_mm_s = ctx->speed_limit_mm_s;
    }
    line_lost_stop = update_line_lost_guard(ctx);
    outputs_allowed = app_safety_outputs_allowed(ctx);
    if (wall_output.drive_allowed == APP_FALSE || line_lost_stop != APP_FALSE) {
        outputs_allowed = APP_FALSE;
    }
    if (ctx->emag.line_lost != APP_FALSE && line_lost_stop == APP_FALSE && outputs_allowed != APP_FALSE) {
        target_speed_mm_s = DIFF_LINE_LOST_SEARCH_SPEED_MM_S;
        ctx->turn_delta_mm_s = 0;
    }
    if (outputs_allowed == APP_FALSE) {
        target_speed_mm_s = DRIVE_SAFE_ZERO;
    }
    ctx->target_speed_mm_s = target_speed_mm_s;
    update_fuzzy_turn(ctx, outputs_allowed);
    if (ctx->emag.line_lost != APP_FALSE) {
        ctx->turn_delta_mm_s = 0;
    }
    diff_output = ctrl_differential_drive_mix(target_speed_mm_s,
                                              ctx->turn_delta_mm_s,
                                              outputs_allowed);
    ctx->left_speed_target_mm_s = diff_output.left_target_mm_s;
    ctx->right_speed_target_mm_s = diff_output.right_target_mm_s;
    if (ctx->encoder.valid == APP_FALSE || outputs_allowed == APP_FALSE) {
        ctrl_speed_pi_reset(&g_left_speed_pi);
        ctrl_speed_pi_reset(&g_right_speed_pi);
    }
    speed_output = ctrl_speed_update_pair(&g_left_speed_pi,
                                          &g_right_speed_pi,
                                          ctx->left_speed_target_mm_s,
                                          ctx->right_speed_target_mm_s,
                                          ctx->encoder);
    g_stats.speed_pi_pair_calls++;
    ctx->left_drive_command_native = speed_output.left_native;
    ctx->right_drive_command_native = speed_output.right_native;
    ctx->drive_command_native = speed_output.average_native;
    ctrl_adhesion_set_physical_active(&g_adhesion_state,
                                      bsp_fan_esc_is_physical_active());
    ctrl_adhesion_update(&g_adhesion_state,
                         ctx->wall_state,
                         ctx->adhesion_risk,
                         dt_ms,
                         &ctx->fan_cmd);
    app_safety_apply_profile(ctx, app_safety_select_profile(ctx));
    reset_fuzzy_if_blocked(ctx);
    app_output_arbitrate(ctx);
    reset_fuzzy_if_blocked(ctx);
    if (outputs_allowed == APP_FALSE) {
        ctrl_speed_pi_reset(&g_left_speed_pi);
        ctrl_speed_pi_reset(&g_right_speed_pi);
    }
    bsp_drive_apply_lr(ctx->left_drive_command_native, ctx->right_drive_command_native);
    bsp_fan_esc_apply(&ctx->fan_cmd);
}

app_control_tick_stats_t app_control_tick_stats(void)
{
    return g_stats;
}

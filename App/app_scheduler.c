#include "app_scheduler.h"
#include "app_config.h"
#include "app_output_arbitration.h"
#include "app_state_machine.h"
#include "app_safety.h"
#include "../BSP/bsp_drive.h"
#include "../BSP/bsp_steering.h"
#include "../BSP/bsp_suction.h"
#include "../BSP/bsp_emag.h"
#include "../BSP/bsp_imu.h"
#include "../BSP/bsp_encoder.h"
#include "../BSP/bsp_ui.h"
#include "../BSP/bsp_power.h"
#include "../Control/ctrl_adhesion.h"
#include "../Control/ctrl_attitude.h"
#include "../Control/ctrl_line.h"
#include "../Control/ctrl_profile.h"
#include "../Control/ctrl_speed.h"
#include "../Control/ctrl_fuzzy_steering.h"
#include "../Control/ctrl_steering.h"
#include "../Control/ctrl_vehicle.h"
#include "../Track/track_features.h"
#include "../Track/track_route_profile.h"
#include "../Track/track_state_machine.h"
#include "../Track/track_strategy.h"
#include "../Track/track_surface_state.h"
#ifdef HOST_SIL
#include "../sim/host/host_bsp.h"
#endif

static u32 g_last_fast;
static u32 g_last_control;
static u32 g_last_track;
static u32 g_last_health;
static u32 g_last_ui;
static track_mode_state_t g_track_mode_state;
static ctrl_fuzzy_steering_state_t g_fuzzy_steering_state;
static line_filter_state_t g_line_filter_state;
static speed_pi_state_t g_left_speed_pi;
static speed_pi_state_t g_right_speed_pi;

static u8 control_or_wall_active(app_state_t state)
{
    return (state == APP_STATE_ARMED_GROUND ||
            state == APP_STATE_GROUND_TRACK ||
            state == APP_STATE_TRANSITION_CANDIDATE ||
            state == APP_STATE_SUCTION_PRECHARGE ||
            state == APP_STATE_APPROACH_TRANSITION ||
            state == APP_STATE_TRANSITION_UP ||
            state == APP_STATE_WALL_TRACK ||
            state == APP_STATE_CYLINDER_TRACK ||
            state == APP_STATE_TRANSITION_DOWN ||
            state == APP_STATE_GROUND_RECOVERY ||
            state == APP_STATE_SEESAW_PASS);
}

static void update_sensor_health(app_context_t *ctx, u32 now_ms)
{
    u32 attitude_age_ms;

    ctx->health.emag_ok = (ctx->emag.valid && ctx->emag.line_lost == APP_FALSE && ctx->emag.line_quality >= LINE_QUALITY_MIN);
    if (ctx->attitude.fresh && now_ms >= ctx->attitude.timestamp_ms) {
        attitude_age_ms = now_ms - ctx->attitude.timestamp_ms;
        ctx->health.imu_fresh = (attitude_age_ms <= SENSOR_STALE_TIMEOUT_MS);
    } else {
        ctx->health.imu_fresh = APP_FALSE;
    }
    ctx->health.encoder_ok = ctx->encoder.valid;
    ctx->health.power_ok = bsp_power_is_ok();
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
    if (control_or_wall_active(ctx->app_state) && ctx->health.emag_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_LINE_LOST);
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

void app_scheduler_init(void)
{
    g_last_fast = 0ul;
    g_last_control = 0ul;
    g_last_track = 0ul;
    g_last_health = 0ul;
    g_last_ui = 0ul;
    ctrl_line_filter_init(&g_line_filter_state);
    ctrl_speed_pi_init(&g_left_speed_pi);
    ctrl_speed_pi_init(&g_right_speed_pi);
    track_route_profile_init(&g_track_mode_state);
    ctrl_fuzzy_steering_init(&g_fuzzy_steering_state);
}

static void update_line_error_terms(app_context_t *ctx)
{
    ctrl_line_filter_update(&g_line_filter_state,
                            &ctx->emag,
                            &ctx->line_error_filtered,
                            &ctx->line_error_rate);
}

static void update_track_mode(app_context_t *ctx)
{
    track_mode_input_t input;
    s32 speed_sum;

    input.line_error = ctx->line_error_filtered;
    input.error_rate = ctx->line_error_rate;
    input.line_quality = ctx->emag.line_quality;
    input.surface_state = ctx->surface_state;
    input.pitch_cdeg = ctx->attitude.pitch_cdeg;
    input.pitch_rate_cdeg_s = 0;
    speed_sum = (s32)ctx->encoder.left_speed_mm_s + (s32)ctx->encoder.right_speed_mm_s;
    input.speed_mm_s = (s16)(speed_sum / 2l);
    input.cross_confirmed = track_features_detect_crossing(&ctx->emag);
    input.hex_confirmed = track_features_detect_hex_loop(&ctx->emag);
    input.seesaw_confirmed = track_features_detect_seesaw(&ctx->attitude);
    input.dt_ms = TASK_CONTROL_PERIOD_MS;

    ctx->track_mode = track_route_profile_select(&g_track_mode_state, &input);
    if (ctx->app_state == APP_STATE_GROUND_RECOVERY) {
        ctx->track_mode = TRACK_MODE_RECOVERY;
    } else if (ctx->app_state == APP_STATE_SEESAW_PASS) {
        ctx->track_mode = TRACK_MODE_SEESAW;
    }
}

static void update_fuzzy_steering(app_context_t *ctx)
{
    ctrl_fuzzy_steering_input_t input;
    s16 offset;

    input.mode = ctx->track_mode;
    input.app_state = ctx->app_state;
    input.line_error_filtered = ctx->line_error_filtered;
    input.error_rate = ctx->line_error_rate;
    input.signal_quality = ctx->emag.line_quality;
    input.dt_ms = TASK_CONTROL_PERIOD_MS;
    input.outputs_allowed = app_safety_outputs_allowed(ctx);

    offset = ctrl_fuzzy_steering_update(&g_fuzzy_steering_state, &input);
    ctx->steering_offset_us = offset;
    ctx->fuzzy_kp = g_fuzzy_steering_state.gain.kp;
    ctx->fuzzy_ki = g_fuzzy_steering_state.gain.ki;
    ctx->fuzzy_kd = g_fuzzy_steering_state.gain.kd;
}

static void reset_fuzzy_if_blocked(app_context_t *ctx)
{
    if (ctrl_fuzzy_steering_needs_reset(ctx->app_state,
                                        ctx->track_mode,
                                        ctx->emag.line_quality,
                                        app_safety_outputs_allowed(ctx)) != APP_FALSE) {
        ctrl_fuzzy_steering_reset(&g_fuzzy_steering_state);
        ctx->fuzzy_kp = g_fuzzy_steering_state.gain.kp;
        ctx->fuzzy_ki = g_fuzzy_steering_state.gain.ki;
        ctx->fuzzy_kd = g_fuzzy_steering_state.gain.kd;
    }
}

void app_scheduler_run_due(app_context_t *ctx, u32 now_ms)
{
    if (ctx == 0) {
        return;
    }

    if ((now_ms - g_last_fast) >= TASK_FAST_SENSOR_PERIOD_MS) {
        g_last_fast = now_ms;
        ctx->emag = bsp_emag_read();
        ctx->attitude = bsp_imu_read();
        ctx->encoder = bsp_encoder_read();
    }

    if ((now_ms - g_last_control) >= TASK_CONTROL_PERIOD_MS) {
        s16 target_speed_mm_s;
        speed_pi_output_t speed_output;
        u8 feature_transition;

        g_last_control = now_ms;
        ctx->emag = ctrl_line_update(ctx->emag);
        update_line_error_terms(ctx);
        ctx->attitude = ctrl_attitude_update(ctx->attitude, TASK_CONTROL_PERIOD_MS);
        ctx->transition_candidate = read_transition_candidate();
        update_sensor_health(ctx, now_ms);
        ctx->surface_state = track_surface_state_update(ctx->surface_state, &ctx->attitude);
        feature_transition = track_features_detect_transition(&ctx->emag);
        ctx->surface_state = track_state_machine_step(ctx->surface_state, feature_transition);
#ifdef HOST_SIL
        apply_host_fault_injection(ctx);
#endif
        app_state_machine_step(ctx, TASK_CONTROL_PERIOD_MS);
        update_sensor_health(ctx, now_ms);
        update_track_mode(ctx);

        ctx->speed_limit_mm_s = ctrl_profile_speed_limit(ctx->surface_state, ctx->adhesion_risk);
        target_speed_mm_s = track_strategy_target_speed_for_mode(ctx->track_mode);
        if (target_speed_mm_s > ctx->speed_limit_mm_s) {
            target_speed_mm_s = ctx->speed_limit_mm_s;
        }
        if (app_safety_outputs_allowed(ctx) == APP_FALSE) {
            target_speed_mm_s = DRIVE_SAFE_ZERO;
        }
        ctx->target_speed_mm_s = target_speed_mm_s;
        ctx->left_speed_target_mm_s = target_speed_mm_s;
        ctx->right_speed_target_mm_s = target_speed_mm_s;
        if (ctx->encoder.valid == APP_FALSE || app_safety_outputs_allowed(ctx) == APP_FALSE) {
            ctrl_speed_pi_reset(&g_left_speed_pi);
            ctrl_speed_pi_reset(&g_right_speed_pi);
        }
        speed_output = ctrl_speed_update_pair(&g_left_speed_pi,
                                              &g_right_speed_pi,
                                              ctx->left_speed_target_mm_s,
                                              ctx->right_speed_target_mm_s,
                                              ctx->encoder);
        ctx->left_drive_command_native = speed_output.left_native;
        ctx->right_drive_command_native = speed_output.right_native;
        ctx->drive_command_native = speed_output.average_native;
        update_fuzzy_steering(ctx);
        ctrl_vehicle_update(ctx);
        ctrl_adhesion_update(ctx);
        app_safety_apply_profile(ctx, app_safety_select_profile(ctx));
        reset_fuzzy_if_blocked(ctx);
        ctrl_vehicle_update(ctx);
        app_output_arbitrate(ctx);
        reset_fuzzy_if_blocked(ctx);
        bsp_drive_apply_lr(ctx->left_drive_command_native, ctx->right_drive_command_native);
        bsp_steering_apply_pair(ctx->steering_left_pulse_us, ctx->steering_right_pulse_us);
        bsp_suction_apply(&ctx->suction_cmd);
    }

    if ((now_ms - g_last_track) >= TASK_TRACK_PERIOD_MS) {
        g_last_track = now_ms;
    }

    if ((now_ms - g_last_health) >= TASK_HEALTH_PERIOD_MS) {
        g_last_health = now_ms;
        ctx->health.power_ok = bsp_power_is_ok();
        if (control_or_wall_active(ctx->app_state) && !ctx->health.imu_fresh) {
            ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
        }
    }

    if ((now_ms - g_last_ui) >= TASK_UI_TELEMETRY_PERIOD_MS) {
        g_last_ui = now_ms;
        ctx->manual_arm = bsp_ui_manual_arm_requested();
        ctx->manual_suction_authorize = bsp_ui_suction_authorized();
        bsp_ui_show_state(ctx->app_state);
    }
}

#include "app_scheduler.h"
#include "app_config.h"
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
#include "../Control/ctrl_steering.h"
#include "../Control/ctrl_vehicle.h"
#include "../Track/track_features.h"
#include "../Track/track_state_machine.h"
#include "../Track/track_strategy.h"
#include "../Track/track_surface_state.h"

static u32 g_last_fast;
static u32 g_last_control;
static u32 g_last_track;
static u32 g_last_health;
static u32 g_last_ui;

static u8 control_or_wall_active(app_state_t state)
{
    return (state == APP_STATE_ARMED_GROUND ||
            state == APP_STATE_SUCTION_PRECHARGE ||
            state == APP_STATE_APPROACH_TRANSITION ||
            state == APP_STATE_TRANSITION_UP ||
            state == APP_STATE_WALL_TRACK ||
            state == APP_STATE_TRANSITION_DOWN ||
            state == APP_STATE_GROUND_RECOVERY);
}

static void update_sensor_health(app_context_t *ctx, u32 now_ms)
{
    u32 attitude_age_ms;

    ctx->health.emag_ok = (ctx->emag.valid && ctx->emag.signal_quality >= LINE_QUALITY_MIN);
    if (ctx->attitude.fresh && now_ms >= ctx->attitude.timestamp_ms) {
        attitude_age_ms = now_ms - ctx->attitude.timestamp_ms;
        ctx->health.imu_fresh = (attitude_age_ms <= SENSOR_STALE_TIMEOUT_MS);
    } else {
        ctx->health.imu_fresh = APP_FALSE;
    }
    ctx->health.encoder_ok = ctx->encoder.valid;
    ctx->health.power_ok = bsp_power_is_ok();
    ctx->health.control_period_ok = APP_TRUE;
    ctx->health.suction_feedback_ok = SUCTION_FEEDBACK_AVAILABLE;

    if (ctx->health.power_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_HARD_POWER);
    }
    if (control_or_wall_active(ctx->app_state) && ctx->health.emag_ok == APP_FALSE) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_LINE_LOST);
    }
    if ((ctx->app_state == APP_STATE_TRANSITION_UP ||
         ctx->app_state == APP_STATE_WALL_TRACK ||
         ctx->app_state == APP_STATE_TRANSITION_DOWN) &&
        (ctx->health.imu_fresh == APP_FALSE || ctx->health.encoder_ok == APP_FALSE)) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
    }
    if (control_or_wall_active(ctx->app_state) && ctx->attitude.pitch_cdeg < -GROUND_PITCH_MAX_CDEG) {
        ctx->faults = (fault_code_t)(ctx->faults | FAULT_SENSOR_STALE);
    }
}

void app_scheduler_init(void)
{
    g_last_fast = 0ul;
    g_last_control = 0ul;
    g_last_track = 0ul;
    g_last_health = 0ul;
    g_last_ui = 0ul;
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
        u8 feature_transition;

        g_last_control = now_ms;
        ctx->emag = ctrl_line_update(ctx->emag);
        ctx->attitude = ctrl_attitude_update(ctx->attitude, TASK_CONTROL_PERIOD_MS);
        update_sensor_health(ctx, now_ms);
        ctx->surface_state = track_surface_state_update(ctx->surface_state, &ctx->attitude);
        feature_transition = track_features_detect_transition(&ctx->emag);
        ctx->surface_state = track_state_machine_step(ctx->surface_state, feature_transition);
        app_state_machine_step(ctx, TASK_CONTROL_PERIOD_MS);
        update_sensor_health(ctx, now_ms);

        ctx->speed_limit_mm_s = ctrl_profile_speed_limit(ctx->surface_state, ctx->adhesion_risk);
        target_speed_mm_s = track_strategy_target_speed(ctx->surface_state);
        if (target_speed_mm_s > ctx->speed_limit_mm_s) {
            target_speed_mm_s = ctx->speed_limit_mm_s;
        }
        if (app_safety_outputs_allowed(ctx) == APP_FALSE) {
            target_speed_mm_s = DRIVE_SAFE_ZERO;
        }
        ctx->drive_command_native = ctrl_speed_command_native(target_speed_mm_s, ctx->encoder);
        ctx->steering_offset_us = ctrl_steering_offset_us(ctx->emag.line_error, ctx->emag.signal_quality);
        ctrl_vehicle_update(ctx);
        ctrl_adhesion_update(ctx);
        app_safety_apply_profile(ctx, app_safety_select_profile(ctx));
        ctrl_vehicle_update(ctx);
        bsp_drive_apply(ctx->drive_command_native);
        bsp_steering_apply(ctx->steering_pulse_us);
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

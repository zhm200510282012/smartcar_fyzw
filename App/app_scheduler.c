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
#include "../Control/ctrl_line.h"
#include "../Control/ctrl_profile.h"

static u32 g_last_fast;
static u32 g_last_control;
static u32 g_last_track;
static u32 g_last_health;
static u32 g_last_ui;

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
        g_last_control = now_ms;
        ctx->emag = ctrl_line_update(ctx->emag);
        ctx->speed_limit_mm_s = ctrl_profile_speed_limit(ctx->surface_state, ctx->adhesion_risk);
        ctrl_adhesion_update(ctx);
        app_safety_apply_profile(ctx, app_safety_select_profile(ctx));
        bsp_drive_apply(ctx->drive_cmd);
        bsp_steering_apply(ctx->steering_cmd);
        bsp_suction_apply(&ctx->suction_cmd);
    }

    if ((now_ms - g_last_track) >= TASK_TRACK_PERIOD_MS) {
        g_last_track = now_ms;
        app_state_machine_step(ctx, TASK_TRACK_PERIOD_MS);
    }

    if ((now_ms - g_last_health) >= TASK_HEALTH_PERIOD_MS) {
        g_last_health = now_ms;
        ctx->health.power_ok = bsp_power_is_ok();
        if (!ctx->health.imu_fresh) {
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

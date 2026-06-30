#include "app_scheduler.h"
#include "app_control_tick.h"
#include "app_config.h"
#include "app_memory.h"
#include "../BSP/bsp_power.h"
#include "../BSP/bsp_ui.h"

static u32 APP_XDATA g_last_sensor;
static u32 APP_XDATA g_last_control;
static u32 APP_XDATA g_last_track;
static u32 APP_XDATA g_last_health;
static u32 APP_XDATA g_last_ui;

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

void app_scheduler_init(void)
{
    g_last_sensor = 0ul;
    g_last_control = 0ul;
    g_last_track = 0ul;
    g_last_health = 0ul;
    g_last_ui = 0ul;
    app_control_tick_init();
}

void app_scheduler_run_due(app_context_t *ctx, u32 now_ms)
{
    if (ctx == 0) {
        return;
    }

#ifdef HOST_SIL
    if ((now_ms - g_last_sensor) >= TASK_FAST_SENSOR_PERIOD_MS) {
        g_last_sensor = now_ms;
        app_control_tick_sensor_isr(ctx, now_ms);
    }

    if ((now_ms - g_last_control) >= TASK_CONTROL_PERIOD_MS) {
        g_last_control = now_ms;
        app_control_tick_control_isr(ctx, now_ms);
    }
#else
    g_last_sensor = now_ms;
    g_last_control = now_ms;
#endif

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

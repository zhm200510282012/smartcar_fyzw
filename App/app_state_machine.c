#include "app_state_machine.h"
#include "app_config.h"
#include "app_safety.h"

static u8 sensors_ready(const app_context_t *ctx)
{
    return (ctx->health.emag_ok && ctx->health.imu_fresh && ctx->health.encoder_ok && ctx->health.control_period_ok);
}

void app_state_machine_init(app_context_t *ctx)
{
    if (ctx == 0) {
        return;
    }
    ctx->app_state = APP_STATE_BOOT;
    ctx->surface_state = SURFACE_UNKNOWN;
    ctx->faults = FAULT_NONE;
    ctx->state_elapsed_ms = 0u;
    ctx->manual_arm = APP_FALSE;
    ctx->manual_suction_authorize = APP_FALSE;
    ctx->test_mode = APP_TEST_MODE;
    ctx->suction_cmd.mode = SUCTION_OFF;
    ctx->suction_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
    ctx->suction_cmd.armed = APP_FALSE;
    ctx->suction_cmd.hw_verified = SUCTION_HW_VERIFIED;
    ctx->suction_cmd.feedback_valid = APP_FALSE;
}

void app_state_machine_step(app_context_t *ctx, u16 dt_ms)
{
    safety_profile_t profile;
    if (ctx == 0) {
        return;
    }

    ctx->state_elapsed_ms = (u16)(ctx->state_elapsed_ms + dt_ms);

    profile = app_safety_select_profile(ctx);
    if (profile != SAFETY_PROFILE_NONE) {
        app_safety_apply_profile(ctx, profile);
        return;
    }

    switch (ctx->app_state) {
    case APP_STATE_BOOT:
        ctx->app_state = APP_STATE_SELF_TEST;
        ctx->state_elapsed_ms = 0u;
        break;
    case APP_STATE_SELF_TEST:
        ctx->app_state = APP_STATE_SENSOR_CALIBRATION;
        ctx->state_elapsed_ms = 0u;
        break;
    case APP_STATE_SENSOR_CALIBRATION:
        ctx->app_state = APP_STATE_SAFE_GROUND_READY;
        ctx->surface_state = SURFACE_GROUND;
        ctx->state_elapsed_ms = 0u;
        break;
    case APP_STATE_SAFE_GROUND_READY:
        if (ctx->manual_arm && sensors_ready(ctx)) {
            ctx->app_state = APP_STATE_ARMED_GROUND;
            ctx->state_elapsed_ms = 0u;
        }
        break;
    case APP_STATE_ARMED_GROUND:
        if (ctx->manual_suction_authorize && SUCTION_HW_VERIFIED) {
            ctx->app_state = APP_STATE_SUCTION_PRECHARGE;
            ctx->state_elapsed_ms = 0u;
        }
        break;
    case APP_STATE_SUCTION_PRECHARGE:
        if (ctx->state_elapsed_ms > WALL_CONFIRM_TIME_MS) {
            ctx->app_state = APP_STATE_APPROACH_TRANSITION;
            ctx->state_elapsed_ms = 0u;
        }
        break;
    case APP_STATE_APPROACH_TRANSITION:
        if (ctx->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            ctx->faults = FAULT_SENSOR_STALE;
        }
        break;
    case APP_STATE_WALL_FAILSAFE_HOLD:
        break;
    default:
        break;
    }
}

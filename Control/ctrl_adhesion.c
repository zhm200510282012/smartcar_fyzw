#include "ctrl_adhesion.h"
#include "../App/app_build_profile.h"
#include "../App/app_config.h"

static u16 logical_suction_request(u16 real_native, u16 host_logical)
{
    if (HOST_SIL_LOGICAL_SUCTION_AVAILABLE != 0) {
        return host_logical;
    }
    return real_native;
}

static u16 base_by_surface(surface_state_t surface)
{
    if (surface == SURFACE_WALL) return logical_suction_request(SUCTION_HOLD_NATIVE, APP_LOGICAL_SUCTION_HOLD_REQUEST);
    if (surface == SURFACE_TRANSITION_UP || surface == SURFACE_TRANSITION_DOWN) {
        return logical_suction_request(SUCTION_PRECHARGE_NATIVE, APP_LOGICAL_SUCTION_PRECHARGE_REQUEST);
    }
    return logical_suction_request(SUCTION_IDLE_NATIVE, SUCTION_IDLE_NATIVE);
}

static u16 estimate_adhesion_risk(const app_context_t *ctx)
{
    u16 risk = 0u;
    if (ctx->surface_state == SURFACE_UNKNOWN) risk = (u16)(risk + 300u);
    if (ctx->health.imu_fresh == APP_FALSE) risk = (u16)(risk + 250u);
    if (ctx->emag.signal_quality < LINE_QUALITY_MIN) risk = (u16)(risk + 250u);
    if (ctx->health.encoder_ok == APP_FALSE) risk = (u16)(risk + 100u);
    if (ctx->health.power_ok == APP_FALSE) risk = (u16)(risk + 100u);
    if (risk > 1000u) risk = 1000u;
    return risk;
}

void ctrl_adhesion_update(app_context_t *ctx)
{
    u16 request;
    if (ctx == 0) {
        return;
    }

    ctx->adhesion_risk = estimate_adhesion_risk(ctx);
    request = base_by_surface(ctx->surface_state);

    if (ctx->app_state == APP_STATE_SUCTION_PRECHARGE ||
        ctx->app_state == APP_STATE_APPROACH_TRANSITION) {
        ctx->suction_cmd.mode = SUCTION_PRECHARGE;
        request = logical_suction_request(SUCTION_PRECHARGE_NATIVE, APP_LOGICAL_SUCTION_PRECHARGE_REQUEST);
    } else if (ctx->app_state == APP_STATE_WALL_TRACK) {
        ctx->suction_cmd.mode = SUCTION_HOLD;
        request = logical_suction_request(SUCTION_HOLD_NATIVE, APP_LOGICAL_SUCTION_HOLD_REQUEST);
    } else if (ctx->adhesion_risk > ADHESION_RISK_LIMIT) {
        ctx->suction_cmd.mode = SUCTION_BOOST;
        request = logical_suction_request(SUCTION_BOOST_NATIVE, APP_LOGICAL_SUCTION_BOOST_REQUEST);
    } else {
        ctx->suction_cmd.mode = SUCTION_IDLE;
    }

    if (ctx->app_state == APP_STATE_WALL_FAILSAFE_HOLD) {
        ctx->suction_cmd.mode = SUCTION_EMERGENCY_HOLD;
        request = logical_suction_request(SUCTION_EMERGENCY_HOLD_NATIVE, APP_LOGICAL_SUCTION_EMERGENCY_REQUEST);
    }

    if (ctx->app_state == APP_STATE_SUCTION_LOCKOUT) {
        ctx->suction_cmd.mode = SUCTION_OFF;
        request = SUCTION_SAFE_OFF_NATIVE;
    }

    ctx->suction_cmd.command_native = request;
    ctx->suction_cmd.armed = ctx->manual_suction_authorize;
    ctx->suction_cmd.hw_verified = SUCTION_HW_VERIFIED;
    ctx->suction_cmd.feedback_valid = SUCTION_FEEDBACK_AVAILABLE;
}

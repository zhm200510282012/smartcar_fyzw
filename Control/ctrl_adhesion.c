#include "ctrl_adhesion.h"
#include "../App/app_config.h"
#include "../BSP/board_map.h"

static void ctrl_adhesion_fill_command(fan_esc_command_t *out,
                                       fan_esc_state_t state,
                                       u16 request_us)
{
    u8 output_allowed;

    if (out == 0) {
        return;
    }

    out->state = state;
    out->request_us = request_us;
    out->mapped = BOARD_FAN_ESC_SIGNAL_MAPPED;
    out->bench_verified = BOARD_FAN_ESC_BENCH_VERIFIED;

    output_allowed = APP_FALSE;
    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0) {
        if (BOARD_FAN_ESC_SIGNAL_MAPPED != 0 &&
            BOARD_FAN_ESC_BENCH_VERIFIED != 0) {
            output_allowed = APP_TRUE;
        }
    }

    if (output_allowed != APP_FALSE) {
        out->output_us = request_us;
    } else {
        out->output_us = 0u;
    }

    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0 &&
        BOARD_FAN_ESC_BENCH_VERIFIED != 0) {
        out->physical_enabled = APP_TRUE;
    } else {
        out->physical_enabled = APP_FALSE;
    }
}

void ctrl_adhesion_init(ctrl_adhesion_state_t *state)
{
    if (state == 0) {
        return;
    }

    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0) {
        state->state = FAN_ESC_ARMING;
    } else {
        state->state = FAN_ESC_OFF;
    }
    state->state_elapsed_ms = 0u;
    state->boost_elapsed_ms = 0u;
    state->ground_confirm_ms = 0u;
    state->ramp_elapsed_ms = 0u;
    state->request_us = FAN_ESC_MIN_US;
    state->physical_active = APP_FALSE;
    state->real_esc_armed = APP_FALSE;
}

void ctrl_adhesion_reset(ctrl_adhesion_state_t *state)
{
    ctrl_adhesion_init(state);
}

void ctrl_adhesion_set_physical_active(ctrl_adhesion_state_t *state, u8 physical_active)
{
    if (state == 0) {
        return;
    }
    state->physical_active = physical_active;
}

static void enter_state(ctrl_adhesion_state_t *state, fan_esc_state_t next, u16 request_us)
{
    if (state->state != next) {
        state->state = next;
        state->state_elapsed_ms = 0u;
        state->boost_elapsed_ms = 0u;
        state->ramp_elapsed_ms = 0u;
    }
    state->request_us = request_us;
}

static u8 handle_null_state(ctrl_adhesion_state_t *state, fan_esc_command_t *out)
{
    if (state != 0) {
        return APP_FALSE;
    }
    ctrl_adhesion_fill_command(out, FAN_ESC_OFF, FAN_ESC_MIN_US);
    return APP_TRUE;
}

static u8 handle_arming(ctrl_adhesion_state_t *state,
                        u16 dt_ms,
                        fan_esc_command_t *out)
{
    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE == 0 ||
        BOARD_FAN_ESC_BENCH_VERIFIED == 0 ||
        state->state != FAN_ESC_ARMING) {
        state->state_elapsed_ms = (u16)(state->state_elapsed_ms + dt_ms);
        return APP_FALSE;
    }

    state->request_us = FAN_ESC_MIN_US;
    if (state->physical_active != APP_FALSE) {
        state->state_elapsed_ms = (u16)(state->state_elapsed_ms + dt_ms);
    }
    if (state->state_elapsed_ms < FAN_ESC_ARM_TIME_MS) {
        ctrl_adhesion_fill_command(out, FAN_ESC_ARMING, FAN_ESC_MIN_US);
        return APP_TRUE;
    }

    state->real_esc_armed = APP_TRUE;
    enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
    return APP_FALSE;
}

static u8 handle_failsafe(ctrl_adhesion_state_t *state,
                          track_wall_state_t wall_state,
                          fan_esc_command_t *out)
{
    if (wall_state != TRACK_WALL_FAILSAFE_HOLD) {
        return APP_FALSE;
    }
    enter_state(state, FAN_ESC_FAILSAFE_HOLD, FAN_HOLD_US);
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
    return APP_TRUE;
}

static u8 handle_ground_or_approach(ctrl_adhesion_state_t *state,
                                    track_wall_state_t wall_state,
                                    fan_esc_command_t *out)
{
    if (wall_state != TRACK_WALL_GROUND_TRACK &&
        wall_state != TRACK_WALL_WALL_APPROACH) {
        return APP_FALSE;
    }
    state->ground_confirm_ms = 0u;
    enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
    return APP_TRUE;
}

static u8 handle_precharge(ctrl_adhesion_state_t *state,
                           track_wall_state_t wall_state,
                           fan_esc_command_t *out)
{
    if (wall_state != TRACK_WALL_FAN_PRECHARGE) {
        return APP_FALSE;
    }
    state->ground_confirm_ms = 0u;
    enter_state(state, FAN_ESC_PRECHARGE, FAN_PRECHARGE_US);
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
    return APP_TRUE;
}

static u8 handle_wall_hold(ctrl_adhesion_state_t *state,
                           track_wall_state_t wall_state,
                           u16 adhesion_risk,
                           u16 dt_ms,
                           fan_esc_command_t *out)
{
    u8 wall_related;

    wall_related = APP_FALSE;
    if (wall_state == TRACK_WALL_TRANSITION_UP ||
        wall_state == TRACK_WALL_WALL_TRACK ||
        wall_state == TRACK_WALL_CYLINDER_TRACK ||
        wall_state == TRACK_WALL_TRANSITION_DOWN) {
        wall_related = APP_TRUE;
    }
    if (wall_related == APP_FALSE) {
        return APP_FALSE;
    }

    state->ground_confirm_ms = 0u;
    if (adhesion_risk > ADHESION_RISK_LIMIT &&
        wall_state != TRACK_WALL_TRANSITION_DOWN &&
        state->state != FAN_ESC_BOOST) {
        enter_state(state, FAN_ESC_BOOST, FAN_BOOST_US);
    } else if (state->state == FAN_ESC_BOOST) {
        state->boost_elapsed_ms = (u16)(state->boost_elapsed_ms + dt_ms);
        if (state->boost_elapsed_ms >= FAN_BOOST_TIME_MS) {
            enter_state(state, FAN_ESC_HOLD, FAN_HOLD_US);
        }
    } else {
        enter_state(state, FAN_ESC_HOLD, FAN_HOLD_US);
    }
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
    return APP_TRUE;
}

static u8 handle_ground_recovery(ctrl_adhesion_state_t *state,
                                 track_wall_state_t wall_state,
                                 u16 dt_ms,
                                 fan_esc_command_t *out)
{
    if (wall_state != TRACK_WALL_GROUND_RECOVERY) {
        return APP_FALSE;
    }

    state->ground_confirm_ms = (u16)(state->ground_confirm_ms + dt_ms);
    if (state->ground_confirm_ms < IMU_GROUND_CONFIRM_MS) {
        enter_state(state, FAN_ESC_HOLD, FAN_HOLD_US);
        ctrl_adhesion_fill_command(out, state->state, state->request_us);
        return APP_TRUE;
    }

    if (state->state != FAN_ESC_RAMP_DOWN) {
        enter_state(state, FAN_ESC_RAMP_DOWN, FAN_HOLD_US);
    }
    state->ramp_elapsed_ms = (u16)(state->ramp_elapsed_ms + dt_ms);
    if (state->ramp_elapsed_ms >= FAN_RAMP_DOWN_PERIOD_MS) {
        state->ramp_elapsed_ms = 0u;
        if (state->request_us > (u16)(FAN_ESC_MIN_US + FAN_RAMP_DOWN_STEP_US)) {
            state->request_us = (u16)(state->request_us - FAN_RAMP_DOWN_STEP_US);
        } else {
            enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
        }
    }
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
    return APP_TRUE;
}

void ctrl_adhesion_update(ctrl_adhesion_state_t *state,
                          track_wall_state_t wall_state,
                          u16 adhesion_risk,
                          u16 dt_ms,
                          fan_esc_command_t *out)
{
    if (handle_null_state(state, out) != APP_FALSE) {
        return;
    }
    if (handle_arming(state, dt_ms, out) != APP_FALSE) {
        return;
    }
    if (handle_failsafe(state, wall_state, out) != APP_FALSE) {
        return;
    }
    if (handle_ground_or_approach(state, wall_state, out) != APP_FALSE) {
        return;
    }
    if (handle_precharge(state, wall_state, out) != APP_FALSE) {
        return;
    }
    if (handle_wall_hold(state, wall_state, adhesion_risk, dt_ms, out) != APP_FALSE) {
        return;
    }
    if (handle_ground_recovery(state, wall_state, dt_ms, out) != APP_FALSE) {
        return;
    }

    enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
    ctrl_adhesion_fill_command(out, state->state, state->request_us);
}

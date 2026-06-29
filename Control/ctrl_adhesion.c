#include "ctrl_adhesion.h"
#include "../App/app_config.h"
#include "../BSP/board_map.h"

static fan_esc_command_t make_command(fan_esc_state_t state, u16 request_us)
{
    fan_esc_command_t cmd;
    cmd.state = state;
    cmd.request_us = request_us;
    cmd.output_us = (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0 && BOARD_FAN_PWM_MAPPED != 0) ? request_us : 0u;
    cmd.mapped = BOARD_FAN_PWM_MAPPED;
    cmd.physical_enabled = FAN_ESC_PHYSICAL_OUTPUT_ENABLE;
    return cmd;
}

void ctrl_adhesion_init(ctrl_adhesion_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->state = (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0) ? FAN_ESC_ARMING : FAN_ESC_OFF;
    state->state_elapsed_ms = 0u;
    state->boost_elapsed_ms = 0u;
    state->ground_confirm_ms = 0u;
    state->ramp_elapsed_ms = 0u;
    state->request_us = FAN_ESC_MIN_US;
}

void ctrl_adhesion_reset(ctrl_adhesion_state_t *state)
{
    ctrl_adhesion_init(state);
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

fan_esc_command_t ctrl_adhesion_update(ctrl_adhesion_state_t *state,
                                       track_wall_state_t wall_state,
                                       u16 adhesion_risk,
                                       u16 dt_ms)
{
    if (state == 0) {
        return make_command(FAN_ESC_OFF, FAN_ESC_MIN_US);
    }

    state->state_elapsed_ms = (u16)(state->state_elapsed_ms + dt_ms);

    if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0 &&
        state->state == FAN_ESC_ARMING) {
        state->request_us = FAN_ESC_MIN_US;
        if (state->state_elapsed_ms < FAN_ESC_ARM_TIME_MS) {
            return make_command(FAN_ESC_ARMING, FAN_ESC_MIN_US);
        }
        enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
    }

    if (wall_state == TRACK_WALL_FAILSAFE_HOLD) {
        enter_state(state, FAN_ESC_FAILSAFE_HOLD, FAN_HOLD_US);
        return make_command(state->state, state->request_us);
    }

    if (wall_state == TRACK_WALL_GROUND_TRACK ||
        wall_state == TRACK_WALL_WALL_APPROACH) {
        state->ground_confirm_ms = 0u;
        enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
        return make_command(state->state, state->request_us);
    }

    if (wall_state == TRACK_WALL_FAN_PRECHARGE) {
        state->ground_confirm_ms = 0u;
        enter_state(state, FAN_ESC_PRECHARGE, FAN_PRECHARGE_US);
        return make_command(state->state, state->request_us);
    }

    if (wall_state == TRACK_WALL_TRANSITION_UP ||
        wall_state == TRACK_WALL_WALL_TRACK ||
        wall_state == TRACK_WALL_CYLINDER_TRACK ||
        wall_state == TRACK_WALL_TRANSITION_DOWN) {
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
        return make_command(state->state, state->request_us);
    }

    if (wall_state == TRACK_WALL_GROUND_RECOVERY) {
        state->ground_confirm_ms = (u16)(state->ground_confirm_ms + dt_ms);
        if (state->ground_confirm_ms < IMU_GROUND_CONFIRM_MS) {
            enter_state(state, FAN_ESC_HOLD, FAN_HOLD_US);
            return make_command(state->state, state->request_us);
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
        return make_command(state->state, state->request_us);
    }

    enter_state(state, FAN_ESC_OFF, FAN_ESC_MIN_US);
    return make_command(state->state, state->request_us);
}

#include "track_route_profile.h"
#include "../Control/ctrl_signal.h"

static const track_route_segment_t g_route_segment_profile[1] = {
    {0u, 0u, TRACK_MODE_RECOVERY, 0}
};

static u16 abs_s16_local(s16 value)
{
    return ctrl_signal_abs_s16(value);
}

static track_mode_t mode_for_surface(surface_state_t surface)
{
    if (surface == SURFACE_TRANSITION_UP || surface == SURFACE_TRANSITION_DOWN) {
        return TRACK_MODE_TRANSITION;
    }
    if (surface == SURFACE_WALL) {
        return TRACK_MODE_WALL;
    }
    if (surface == SURFACE_CYLINDER) {
        return TRACK_MODE_CYLINDER;
    }
    return TRACK_MODE_STRAIGHT;
}

void track_route_profile_init(track_mode_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->active_mode = TRACK_MODE_STRAIGHT;
    state->candidate_mode = TRACK_MODE_STRAIGHT;
    state->candidate_count = 0u;
    state->dwell_ms = 0u;
}

track_mode_t track_route_profile_detect_raw(const track_mode_input_t *input)
{
    u16 abs_error;
    u16 abs_rate;
    track_mode_t surface_mode;

    if (input == 0) {
        return TRACK_MODE_RECOVERY;
    }
    if (input->line_quality < TRACK_LINE_LOST_QUALITY_MIN) {
        return TRACK_MODE_LINE_LOST;
    }

    surface_mode = mode_for_surface(input->surface_state);
    if (surface_mode != TRACK_MODE_STRAIGHT) {
        return surface_mode;
    }

    if (input->seesaw_confirmed != APP_FALSE) {
        return TRACK_MODE_SEESAW;
    }
    if (input->hex_confirmed != APP_FALSE) {
        return TRACK_MODE_HEX_LOOP;
    }
    if (input->cross_confirmed != APP_FALSE) {
        return TRACK_MODE_CROSSING;
    }

    abs_error = abs_s16_local(input->line_error);
    abs_rate = abs_s16_local(input->error_rate);

    if (abs_error >= TRACK_SHARP_CURVE_ERROR_MIN && abs_rate >= TRACK_OMEGA_RATE_MIN) {
        return TRACK_MODE_OMEGA;
    }
    if (abs_error >= TRACK_SHARP_CURVE_ERROR_MIN || abs_rate >= TRACK_OMEGA_RATE_MIN) {
        return TRACK_MODE_SHARP_CURVE;
    }
    if (abs_error > TRACK_STRAIGHT_ERROR_MAX || abs_rate > TRACK_STRAIGHT_RATE_MAX) {
        if (abs_error <= TRACK_NORMAL_CURVE_ERROR_MAX) {
            return TRACK_MODE_NORMAL_CURVE;
        }
        return TRACK_MODE_SHARP_CURVE;
    }
    return TRACK_MODE_STRAIGHT;
}

track_mode_t track_route_profile_select(track_mode_state_t *state, const track_mode_input_t *input)
{
    track_mode_t raw_mode;

    raw_mode = track_route_profile_detect_raw(input);
    if (state == 0) {
        return raw_mode;
    }

    if (input != 0) {
        state->dwell_ms = (u16)(state->dwell_ms + input->dt_ms);
    }

    if (raw_mode == state->active_mode) {
        state->candidate_mode = raw_mode;
        state->candidate_count = 0u;
        return state->active_mode;
    }

    if (raw_mode != state->candidate_mode) {
        state->candidate_mode = raw_mode;
        state->candidate_count = 1u;
        return state->active_mode;
    }

    if (state->candidate_count < 60000u) {
        state->candidate_count++;
    }

    if (state->candidate_count >= TRACK_MODE_DEBOUNCE_COUNT &&
        state->dwell_ms >= TRACK_MODE_MIN_DWELL_MS) {
        state->active_mode = raw_mode;
        state->candidate_count = 0u;
        state->dwell_ms = 0u;
    }

    return state->active_mode;
}

u16 track_route_profile_configured_count(void)
{
    return 0u;
}

const track_route_segment_t *track_route_profile_table(u16 *count)
{
    if (count != 0) {
        *count = 0u;
    }
    return g_route_segment_profile;
}

const char *track_route_profile_mode_name(track_mode_t mode)
{
    switch (mode) {
    case TRACK_MODE_STRAIGHT: return "STRAIGHT";
    case TRACK_MODE_NORMAL_CURVE: return "NORMAL_CURVE";
    case TRACK_MODE_SHARP_CURVE: return "SHARP_CURVE";
    case TRACK_MODE_CROSSING: return "CROSSING";
    case TRACK_MODE_OMEGA: return "OMEGA";
    case TRACK_MODE_HEX_LOOP: return "HEX_LOOP";
    case TRACK_MODE_TRANSITION: return "TRANSITION";
    case TRACK_MODE_WALL: return "WALL";
    case TRACK_MODE_CYLINDER: return "CYLINDER";
    case TRACK_MODE_SEESAW: return "SEESAW";
    case TRACK_MODE_LINE_LOST: return "LINE_LOST";
    case TRACK_MODE_RECOVERY: return "RECOVERY";
    default: return "UNKNOWN";
    }
}

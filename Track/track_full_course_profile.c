#include "track_full_course_profile.h"

static u32 segment_bit(track_course_segment_t segment)
{
    return (u32)(1ul << (u8)segment);
}

static s16 abs_s16_local(s16 value)
{
    if (value < 0) {
        return (s16)-value;
    }
    return value;
}

void track_full_course_profile_init(track_full_course_profile_t *profile)
{
    if (profile == 0) {
        return;
    }
    profile->segment = TRACK_COURSE_START;
    profile->seen_mask = segment_bit(TRACK_COURSE_START);
}

static track_course_segment_t segment_from_wall_state(track_wall_state_t state)
{
    switch (state) {
    case TRACK_WALL_WALL_APPROACH: return TRACK_COURSE_WALL_APPROACH;
    case TRACK_WALL_FAN_PRECHARGE: return TRACK_COURSE_FAN_PRECHARGE;
    case TRACK_WALL_TRANSITION_UP: return TRACK_COURSE_TRANSITION_UP;
    case TRACK_WALL_WALL_TRACK: return TRACK_COURSE_WALL_TRACK;
    case TRACK_WALL_CYLINDER_TRACK: return TRACK_COURSE_CYLINDER_TRACK;
    case TRACK_WALL_TRANSITION_DOWN: return TRACK_COURSE_TRANSITION_DOWN;
    case TRACK_WALL_GROUND_RECOVERY: return TRACK_COURSE_GROUND_RECOVERY;
    case TRACK_WALL_GROUND_TRACK:
    case TRACK_WALL_FAILSAFE_HOLD:
    default:
        return TRACK_COURSE_GROUND_STRAIGHT;
    }
}

static track_course_segment_t segment_from_track_mode(track_mode_t mode)
{
    switch (mode) {
    case TRACK_MODE_NORMAL_CURVE: return TRACK_COURSE_NORMAL_CURVE;
    case TRACK_MODE_SHARP_CURVE: return TRACK_COURSE_SHARP_CURVE;
    case TRACK_MODE_CROSSING: return TRACK_COURSE_CROSSING;
    case TRACK_MODE_OMEGA: return TRACK_COURSE_OMEGA;
    case TRACK_MODE_HEX_LOOP: return TRACK_COURSE_HEX_LOOP;
    case TRACK_MODE_TRANSITION: return TRACK_COURSE_WALL_APPROACH;
    case TRACK_MODE_WALL: return TRACK_COURSE_WALL_TRACK;
    case TRACK_MODE_CYLINDER: return TRACK_COURSE_CYLINDER_TRACK;
    case TRACK_MODE_RECOVERY: return TRACK_COURSE_GROUND_RECOVERY;
    case TRACK_MODE_STRAIGHT:
    default:
        return TRACK_COURSE_GROUND_STRAIGHT;
    }
}

static track_course_segment_t segment_from_line(const track_full_course_input_t *input)
{
    s16 abs_error;
    if (input->line_quality < TRACK_LINE_LOST_QUALITY_MIN) {
        return TRACK_COURSE_GROUND_STRAIGHT;
    }
    abs_error = abs_s16_local(input->line_error_filtered);
    if (abs_error >= TRACK_SHARP_CURVE_ERROR_MIN ||
        abs_s16_local(input->line_error_rate) >= TRACK_OMEGA_RATE_MIN) {
        return TRACK_COURSE_SHARP_CURVE;
    }
    if (abs_error > TRACK_STRAIGHT_ERROR_MAX) {
        return TRACK_COURSE_NORMAL_CURVE;
    }
    return TRACK_COURSE_GROUND_STRAIGHT;
}

track_course_segment_t track_full_course_profile_update(track_full_course_profile_t *profile,
                                                        const track_full_course_input_t *input)
{
    track_course_segment_t segment;

    if (profile == 0) {
        return TRACK_COURSE_START;
    }
    if (input == 0) {
        profile->segment = TRACK_COURSE_START;
        profile->seen_mask |= segment_bit(TRACK_COURSE_START);
        return profile->segment;
    }

    if (input->app_state < APP_STATE_GROUND_TRACK) {
        segment = TRACK_COURSE_START;
    } else if (input->app_state == APP_STATE_FINISHED ||
               input->route_event.finish_event != APP_FALSE) {
        segment = TRACK_COURSE_FINISH;
    } else if (input->wall_state != TRACK_WALL_GROUND_TRACK &&
               input->wall_state != TRACK_WALL_FAILSAFE_HOLD) {
        segment = segment_from_wall_state(input->wall_state);
    } else if (input->route_event.wall_approach_event != APP_FALSE) {
        segment = TRACK_COURSE_WALL_APPROACH;
    } else if (input->route_event.crossing_event != APP_FALSE) {
        segment = TRACK_COURSE_CROSSING;
    } else if (input->route_event.omega_event != APP_FALSE) {
        segment = TRACK_COURSE_OMEGA;
    } else if (input->route_event.hex_loop_event != APP_FALSE) {
        segment = TRACK_COURSE_HEX_LOOP;
    } else {
        segment = segment_from_track_mode(input->track_mode);
        if (segment == TRACK_COURSE_GROUND_STRAIGHT) {
            segment = segment_from_line(input);
        }
    }

    profile->segment = segment;
    profile->seen_mask |= segment_bit(segment);
    return segment;
}

u32 track_full_course_profile_seen_mask(const track_full_course_profile_t *profile)
{
    if (profile == 0) {
        return 0ul;
    }
    return profile->seen_mask;
}

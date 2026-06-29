#ifndef TRACK_ROUTE_PROFILE_H
#define TRACK_ROUTE_PROFILE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef struct {
    u16 start_mm;
    u16 end_mm;
    track_mode_t mode;
    s16 nominal_speed_mm_s;
} track_route_segment_t;

typedef struct {
    s16 line_error;
    s16 error_rate;
    u16 line_quality;
    surface_state_t surface_state;
    s16 pitch_cdeg;
    s16 pitch_rate_cdeg_s;
    s16 speed_mm_s;
    u8 cross_confirmed;
    u8 hex_confirmed;
    u8 seesaw_confirmed;
    u16 dt_ms;
} track_mode_input_t;

typedef struct {
    track_mode_t active_mode;
    track_mode_t candidate_mode;
    u16 candidate_count;
    u16 dwell_ms;
} track_mode_state_t;

void track_route_profile_init(track_mode_state_t *state);
track_mode_t track_route_profile_select(track_mode_state_t *state, const track_mode_input_t *input);
track_mode_t track_route_profile_detect_raw(const track_mode_input_t *input);
u16 track_route_profile_configured_count(void);
const track_route_segment_t *track_route_profile_table(u16 *count);
const char *track_route_profile_mode_name(track_mode_t mode);

#endif

#ifndef TRACK_FEATURES_H
#define TRACK_FEATURES_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef enum {
    TRACK_SPECIAL_NONE = 0,
    TRACK_SPECIAL_ELEMENT_CANDIDATE,
    TRACK_SPECIAL_RIGHT_ANGLE_CANDIDATE,
    TRACK_SPECIAL_RING_CANDIDATE,
    TRACK_SPECIAL_CROSSING_CANDIDATE
} track_special_candidate_t;

typedef struct {
    u8 active_element_mask;
    u8 active_element_count;
    u8 baseline_element_count;
    s8 element_count_rise;
    app_bool_t element_burst;
    app_bool_t element_burst_rising_edge;
    app_bool_t element_burst_armed;
} track_element_feature_t;

void track_features_reset(void);
track_element_feature_t track_features_update_elements(const emag_sample_t *emag, u16 dt_ms);
track_special_candidate_t track_features_special_candidate(void);
u8 track_features_detect_transition(const emag_sample_t *emag);
u8 track_features_detect_crossing(const emag_sample_t *emag);
u8 track_features_detect_hex_loop(const emag_sample_t *emag);
u8 track_features_detect_seesaw(const attitude_sample_t *attitude);

#endif

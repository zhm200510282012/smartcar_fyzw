#ifndef TRACK_FEATURES_H
#define TRACK_FEATURES_H

#include "../App/app_types.h"
#include "../App/app_config.h"

u8 track_features_detect_transition(const emag_sample_t *emag);
u8 track_features_detect_crossing(const emag_sample_t *emag);
u8 track_features_detect_hex_loop(const emag_sample_t *emag);
u8 track_features_detect_seesaw(const attitude_sample_t *attitude);

#endif

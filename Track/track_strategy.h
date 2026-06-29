#ifndef TRACK_STRATEGY_H
#define TRACK_STRATEGY_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef struct {
    track_mode_t mode;
    s16 base_kp;
    s16 base_ki;
    s16 base_kd;
    s16 kp_adjust_limit;
    s16 ki_adjust_limit;
    s16 kd_adjust_limit;
    s16 steering_offset_limit_us;
    s16 steering_rate_limit_us_per_tick;
    s16 target_speed_mm_s;
} track_mode_params_t;

s16 track_strategy_target_speed(surface_state_t surface);
s16 track_strategy_target_speed_for_mode(track_mode_t mode);
const track_mode_params_t *track_strategy_mode_params(track_mode_t mode);

#endif

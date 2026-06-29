#ifndef CTRL_FUZZY_TURN_H
#define CTRL_FUZZY_TURN_H

#include "../App/app_types.h"
#include "../Track/track_strategy.h"
#include "ctrl_fuzzy_pid.h"

typedef struct {
    track_mode_t mode;
    app_state_t app_state;
    s16 line_error_filtered;
    s16 error_rate;
    u16 signal_quality;
    u16 dt_ms;
    u8 outputs_allowed;
} ctrl_fuzzy_turn_input_t;

typedef struct {
    track_mode_t last_mode;
    s16 integral;
    s16 previous_error;
    s16 previous_output_mm_s;
    u16 fuzzy_elapsed_ms;
    u8 gain_valid;
    fuzzy_pid_gain_t gain;
    fuzzy_pid_adjust_t adjust;
} ctrl_fuzzy_turn_state_t;

void ctrl_fuzzy_turn_init(ctrl_fuzzy_turn_state_t *state);
void ctrl_fuzzy_turn_reset(ctrl_fuzzy_turn_state_t *state);
s16 ctrl_fuzzy_turn_update(ctrl_fuzzy_turn_state_t *state,
                           const ctrl_fuzzy_turn_input_t *input);
u8 ctrl_fuzzy_turn_needs_reset(app_state_t app_state,
                               track_mode_t mode,
                               u16 signal_quality,
                               u8 outputs_allowed);

#endif

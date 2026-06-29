#ifndef CTRL_LINE_H
#define CTRL_LINE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef struct {
    u8 valid;
    s16 previous_filtered;
} line_filter_state_t;

emag_sample_t ctrl_line_update(emag_sample_t input);
void ctrl_line_filter_init(line_filter_state_t *state);
void ctrl_line_filter_reset(line_filter_state_t *state);
void ctrl_line_filter_update(line_filter_state_t *state,
                             const emag_sample_t *sample,
                             s16 *filtered_error,
                             s16 *error_rate);

#endif

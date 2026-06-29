#ifndef CTRL_SPEED_H
#define CTRL_SPEED_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef struct {
    s32 integral;
    s16 previous_output;
} speed_pi_state_t;

typedef struct {
    s16 left_native;
    s16 right_native;
    s16 average_native;
} speed_pi_output_t;

void ctrl_speed_pi_init(speed_pi_state_t *state);
void ctrl_speed_pi_reset(speed_pi_state_t *state);
s16 ctrl_speed_pi_update(speed_pi_state_t *state,
                         s16 target_mm_s,
                         s16 measured_mm_s,
                         u8 encoder_valid);
speed_pi_output_t ctrl_speed_update_pair(speed_pi_state_t *left_state,
                                         speed_pi_state_t *right_state,
                                         s16 left_target_mm_s,
                                         s16 right_target_mm_s,
                                         encoder_sample_t encoder);
s16 ctrl_speed_command_native(s16 target_mm_s, encoder_sample_t encoder);

#endif

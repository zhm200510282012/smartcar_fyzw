#ifndef CTRL_ADHESION_H
#define CTRL_ADHESION_H

#include "../App/app_types.h"

typedef struct {
    fan_esc_state_t state;
    u16 state_elapsed_ms;
    u16 boost_elapsed_ms;
    u16 ground_confirm_ms;
    u16 ramp_elapsed_ms;
    u16 request_us;
    u8 physical_active;
    u8 real_esc_armed;
} ctrl_adhesion_state_t;

void ctrl_adhesion_init(ctrl_adhesion_state_t *state);
void ctrl_adhesion_reset(ctrl_adhesion_state_t *state);
void ctrl_adhesion_set_physical_active(ctrl_adhesion_state_t *state, u8 physical_active);
fan_esc_command_t ctrl_adhesion_update(ctrl_adhesion_state_t *state,
                                       track_wall_state_t wall_state,
                                       u16 adhesion_risk,
                                       u16 dt_ms);

#endif

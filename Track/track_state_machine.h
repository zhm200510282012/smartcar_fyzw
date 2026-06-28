#ifndef TRACK_STATE_MACHINE_H
#define TRACK_STATE_MACHINE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

surface_state_t track_state_machine_step(surface_state_t current, u8 feature_transition);

#endif

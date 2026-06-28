#ifndef TRACK_SURFACE_STATE_H
#define TRACK_SURFACE_STATE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

surface_state_t track_surface_state_update(surface_state_t current, const attitude_sample_t *attitude);

#endif

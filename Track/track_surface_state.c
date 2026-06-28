#include "track_surface_state.h"

surface_state_t track_surface_state_update(surface_state_t current, const attitude_sample_t *attitude)
{
    if (attitude == 0 || attitude->fresh == APP_FALSE) return SURFACE_UNKNOWN;
    return current;
}

#include "track_surface_state.h"

surface_state_t track_surface_state_update(surface_state_t current, const attitude_sample_t *attitude)
{
    if (attitude == 0 || attitude->fresh == APP_FALSE) return SURFACE_UNKNOWN;
    if (attitude->pitch_cdeg >= WALL_PITCH_CDEG) return SURFACE_WALL;
    if (current == SURFACE_WALL && attitude->pitch_cdeg > GROUND_PITCH_MAX_CDEG) return SURFACE_TRANSITION_DOWN;
    if (attitude->pitch_cdeg >= TRANSITION_PITCH_CDEG) return SURFACE_TRANSITION_UP;
    if (attitude->pitch_cdeg <= -TRANSITION_PITCH_CDEG) return SURFACE_CYLINDER;
    if (attitude->pitch_cdeg <= GROUND_PITCH_MAX_CDEG && attitude->pitch_cdeg >= -GROUND_PITCH_MAX_CDEG) return SURFACE_GROUND;
    return current;
}

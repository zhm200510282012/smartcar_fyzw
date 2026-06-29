#include "track_surface_state.h"

surface_state_t track_surface_state_update(surface_state_t current, const attitude_sample_t *attitude)
{
    s32 pitch;
    if (attitude == 0 || attitude->fresh == APP_FALSE) return SURFACE_UNKNOWN;
    pitch = ((s32)IMU_PITCH_SIGN * (s32)attitude->pitch_cdeg) +
            (s32)IMU_PITCH_OFFSET_CDEG;
    if (pitch >= IMU_WALL_ENTER_CDEG) return SURFACE_WALL;
    if (current == SURFACE_WALL && pitch > IMU_WALL_EXIT_CDEG) return SURFACE_WALL;
    if (current == SURFACE_WALL && pitch <= IMU_WALL_EXIT_CDEG) return SURFACE_TRANSITION_DOWN;
    if (pitch >= TRANSITION_PITCH_CDEG) return SURFACE_TRANSITION_UP;
    if (pitch <= -TRANSITION_PITCH_CDEG) return SURFACE_CYLINDER;
    if (pitch <= GROUND_PITCH_MAX_CDEG && pitch >= -GROUND_PITCH_MAX_CDEG) return SURFACE_GROUND;
    return current;
}

#include "track_strategy.h"

s16 track_strategy_target_speed(surface_state_t surface)
{
    if (surface == SURFACE_WALL) return SPEED_LIMIT_WALL_MM_S;
    if (surface == SURFACE_TRANSITION_UP) return SPEED_LIMIT_TRANSITION_MM_S;
    return SPEED_LIMIT_GROUND_MM_S;
}

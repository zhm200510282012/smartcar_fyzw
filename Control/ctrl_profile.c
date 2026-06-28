#include "ctrl_profile.h"

s16 ctrl_profile_speed_limit(surface_state_t surface, u16 adhesion_risk)
{
    s16 limit;
    if (surface == SURFACE_WALL) limit = SPEED_LIMIT_WALL_MM_S;
    else if (surface == SURFACE_TRANSITION_UP || surface == SURFACE_TRANSITION_DOWN) limit = SPEED_LIMIT_TRANSITION_MM_S;
    else limit = SPEED_LIMIT_GROUND_MM_S;
    if (adhesion_risk > ADHESION_RISK_LIMIT && limit > SPEED_LIMIT_PRECHARGE_MM_S) {
        limit = SPEED_LIMIT_PRECHARGE_MM_S;
    }
    return limit;
}

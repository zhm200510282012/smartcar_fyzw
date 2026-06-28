#include "track_state_machine.h"

static volatile u8 g_last_feature_transition;

surface_state_t track_state_machine_step(surface_state_t current, u8 feature_transition)
{
    g_last_feature_transition = feature_transition;
    return current;
}

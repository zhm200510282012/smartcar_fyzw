#ifndef APP_SAFETY_H
#define APP_SAFETY_H

#include "app_types.h"

typedef enum {
    SAFETY_PROFILE_NONE = 0,
    SAFETY_PROFILE_GROUND_FAULT,
    SAFETY_PROFILE_WALL_OR_UNKNOWN_FAULT,
    SAFETY_PROFILE_SUCTION_LOCKOUT,
    SAFETY_PROFILE_HARD_POWER_OR_THERMAL_FAULT
} safety_profile_t;

safety_profile_t app_safety_select_profile(const app_context_t *ctx);
void app_safety_apply_profile(app_context_t *ctx, safety_profile_t profile);
u8 app_safety_outputs_allowed(const app_context_t *ctx);

#endif

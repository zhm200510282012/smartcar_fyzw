#include "ctrl_transition.h"

surface_state_t ctrl_transition_estimate_surface(const app_context_t *ctx)
{
    if (ctx == 0) return SURFACE_UNKNOWN;
    if (ctx->attitude.fresh == APP_FALSE) return SURFACE_UNKNOWN;
    return ctx->surface_state;
}

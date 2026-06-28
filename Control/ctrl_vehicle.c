#include "ctrl_vehicle.h"

void ctrl_vehicle_update(app_context_t *ctx)
{
    if (ctx == 0) return;
    if (ctx->speed_limit_mm_s <= 0) {
        ctx->drive_cmd = 0;
    }
}

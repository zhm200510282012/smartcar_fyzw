#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include "app_types.h"

void app_state_machine_init(app_context_t *ctx);
void app_state_machine_step(app_context_t *ctx, u16 dt_ms);

#endif

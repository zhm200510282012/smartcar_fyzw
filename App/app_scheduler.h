#ifndef APP_SCHEDULER_H
#define APP_SCHEDULER_H

#include "app_types.h"

void app_scheduler_init(void);
void app_scheduler_run_due(app_context_t *ctx, u32 now_ms);

#endif

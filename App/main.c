#include "app_types.h"
#include "app_state_machine.h"
#include "app_scheduler.h"
#include "../BSP/bsp_init.h"
#include "../BSP/bsp_timebase.h"

static app_context_t g_app;

int main(void)
{
    bsp_init_all_safe();
    app_state_machine_init(&g_app);
    app_scheduler_init();

    for (;;) {
        app_scheduler_run_due(&g_app, bsp_timebase_now_ms());
    }
}

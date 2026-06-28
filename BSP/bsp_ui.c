#include "bsp_ui.h"

static volatile app_state_t g_last_shown_state;

void bsp_ui_init(void)
{
    g_last_shown_state = APP_STATE_BOOT;
}

u8 bsp_ui_manual_arm_requested(void)
{
    return APP_FALSE;
}

u8 bsp_ui_suction_authorized(void)
{
    return APP_FALSE;
}

void bsp_ui_show_state(app_state_t state)
{
    g_last_shown_state = state;
}

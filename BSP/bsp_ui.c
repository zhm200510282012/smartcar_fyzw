#include "bsp_ui.h"

void bsp_ui_init(void)
{
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
    (void)state;
}

#ifndef BSP_UI_H
#define BSP_UI_H

#include "../App/app_types.h"

void bsp_ui_init(void);
u8 bsp_ui_manual_arm_requested(void);
u8 bsp_ui_suction_authorized(void);
void bsp_ui_show_state(app_state_t state);

#endif

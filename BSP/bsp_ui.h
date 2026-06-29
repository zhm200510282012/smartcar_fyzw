#ifndef BSP_UI_H
#define BSP_UI_H

#include "../App/app_types.h"

void bsp_ui_init(void);
app_bool_t bsp_ui_manual_arm_requested(void);
app_bool_t bsp_ui_suction_authorized(void);
void bsp_ui_show_state(app_state_t state);

#ifdef HOST_SIL
void bsp_ui_host_set_arm(app_bool_t requested);
void bsp_ui_host_set_suction_authorized(app_bool_t authorized);
#endif

#endif

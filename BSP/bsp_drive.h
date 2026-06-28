#ifndef BSP_DRIVE_H
#define BSP_DRIVE_H

#include "../App/app_types.h"

void bsp_drive_init(void);
void bsp_drive_apply(s16 drive_command_native);
s16 bsp_drive_last_command_native(void);

#endif

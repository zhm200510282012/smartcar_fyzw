#include "bsp_drive.h"
#include "../App/app_config.h"
#include "board_map.h"

static s16 g_drive_command;

void bsp_drive_init(void)
{
    g_drive_command = DRIVE_SAFE_ZERO;
}

void bsp_drive_apply(s16 command)
{
    if (BOARD_DRIVE_VERIFIED == 0) {
        g_drive_command = DRIVE_SAFE_ZERO;
        return;
    }
    if (command > DRIVE_LIMIT_ABS) command = DRIVE_LIMIT_ABS;
    if (command < -DRIVE_LIMIT_ABS) command = -DRIVE_LIMIT_ABS;
    g_drive_command = command;
}

s16 bsp_drive_last_command(void)
{
    return g_drive_command;
}

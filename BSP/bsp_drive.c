#include "bsp_drive.h"
#include "../App/app_config.h"
#include "board_map.h"

static s16 g_drive_command_native;

void bsp_drive_init(void)
{
    g_drive_command_native = DRIVE_SAFE_ZERO;
}

void bsp_drive_apply(s16 drive_command_native)
{
    if (BOARD_DRIVE_VERIFIED == 0) {
        g_drive_command_native = DRIVE_SAFE_ZERO;
        return;
    }
    if (drive_command_native > DRIVE_LIMIT_ABS) drive_command_native = DRIVE_LIMIT_ABS;
    if (drive_command_native < -DRIVE_LIMIT_ABS) drive_command_native = -DRIVE_LIMIT_ABS;
    g_drive_command_native = drive_command_native;
}

s16 bsp_drive_last_command_native(void)
{
    return g_drive_command_native;
}

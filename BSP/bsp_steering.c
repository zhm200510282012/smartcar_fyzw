#include "bsp_steering.h"
#include "../App/app_config.h"
#include "board_map.h"

static s16 g_steering_command;

void bsp_steering_init(void)
{
    g_steering_command = STEERING_SAFE_CENTER;
}

void bsp_steering_apply(s16 command)
{
    if (BOARD_STEERING_VERIFIED == 0) {
        g_steering_command = STEERING_SAFE_CENTER;
        return;
    }
    if (command > STEERING_LIMIT_ABS) command = STEERING_LIMIT_ABS;
    if (command < -STEERING_LIMIT_ABS) command = -STEERING_LIMIT_ABS;
    g_steering_command = command;
}

s16 bsp_steering_last_command(void)
{
    return g_steering_command;
}
